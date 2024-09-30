/**********************************************************************************
*   Created 2018 by Johny Pham
*   tuaanpv@gmail.com
*   Youtube/Tiktok/Instagram: tuaanpv
***********************************************************************************/

#ifndef __FACE_RECOG_H__
#define __FACE_RECOG_H__

#include <opencv2/dnn.hpp>
#include <queue>

#include "Buffer.h"
#include "Detection.h"


#include <dlib/opencv.h>
#include <dlib/opencv/cv_image.h>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing/render_face_detections.h>
#include <dlib/svm_threaded.h>
#include <dlib/gui_widgets.h>
#include <dlib/image_processing.h>
#include <dlib/data_io.h>
#include <dlib/image_transforms.h>
#include <dlib/cmd_line_parser.h>
#include <dlib/dir_nav.h>
#include <dlib/clustering.h>
#include <dlib/string.h>
#include <dlib/dnn.h>
#include <dlib/image_io.h>
#include <dlib/matrix.h>
#include <dlib/sparse_vector.h>
#include <dlib/geometry.h>
#include <dlib/queue.h>

#include <time.h>
#include "Buffer.h"
#include "utils.h"

using namespace dlib;
using namespace std;

// ----------------------------------------------------------------------------------------

// The next bit of code defines a ResNet network.  It's basically copied
// and pasted from the dnn_imagenet_ex.cpp example, except we replaced the loss
// layer with loss_metric and made the network somewhat smaller.  Go read the introductory
// dlib DNN examples to learn what all this stuff means.
//
// Also, the dnn_metric_learning_on_images_ex.cpp example shows how to train this network.
// The dlib_face_recognition_resnet_model_v1 model used by this example was trained using
// essentially the code shown in dnn_metric_learning_on_images_ex.cpp except the
// mini-batches were made larger (35x15 instead of 5x5), the iterations without progress
// was set to 10000, and the training dataset consisted of about 3 million images instead of
// 55.  Also, the input layer was locked to images of size 150.
template <template <int,template<typename>class,int,typename> class block, int N, template<typename>class BN, typename SUBNET>
using residual = add_prev1<block<N,BN,1,tag1<SUBNET>>>;

template <template <int,template<typename>class,int,typename> class block, int N, template<typename>class BN, typename SUBNET>
using residual_down = add_prev2<avg_pool<2,2,2,2,skip1<tag2<block<N,BN,2,tag1<SUBNET>>>>>>;

template <int N, template <typename> class BN, int stride, typename SUBNET> 
using block  = BN<con<N,3,3,1,1,relu<BN<con<N,3,3,stride,stride,SUBNET>>>>>;

template <int N, typename SUBNET> using ares      = relu<residual<block,N,affine,SUBNET>>;
template <int N, typename SUBNET> using ares_down = relu<residual_down<block,N,affine,SUBNET>>;

template <typename SUBNET> using alevel0 = ares_down<256,SUBNET>;
template <typename SUBNET> using alevel1 = ares<256,ares<256,ares_down<256,SUBNET>>>;
template <typename SUBNET> using alevel2 = ares<128,ares<128,ares_down<128,SUBNET>>>;
template <typename SUBNET> using alevel3 = ares<64,ares<64,ares<64,ares_down<64,SUBNET>>>>;
template <typename SUBNET> using alevel4 = ares<32,ares<32,ares<32,SUBNET>>>;

using anet_type = loss_metric<fc_no_bias<128,avg_pool_everything<
                            alevel0<
                            alevel1<
                            alevel2<
                            alevel3<
                            alevel4<
                            max_pool<3,3,2,2,relu<affine<con<32,7,7,2,2,
                            input_rgb_image_sized<150>
                            >>>>>>>>>>>>;

// ----------------------------------------------------------------------------------------

class FaceRecog
{
    public:
        FaceRecog()
        {
            dlib::deserialize(_file_faces_name) >> _faces_name;
            dlib::deserialize(_file_faces_database) >> _faces_database;
            dlib::deserialize("../model/dlib/shape_predictor_68_face_landmarks.dat") >> _sp;
            dlib::deserialize("../model/dlib/dlib_face_recognition_resnet_model_v1.dat") >> _face_net;

            // if(!_faces_manager_thread.joinable())
            //     _faces_manager_thread = std::thread(&FaceRecog::faceManager, this);
        }

        void setWSQueue(std::shared_ptr<sBuffer<std::string>> put_msg)
        {
            _put_ws_msg = put_msg;
        }

        ~FaceRecog(){};

        void inClassID(size_t classID)
        {
            _face_in_classID = classID;
        }

        int findFaces(const cv::Mat &image, std::vector<RectOut> &rects)
        {
            dlib::matrix<dlib::rgb_pixel> cimg = dlib_image(image);
            std::vector<matrix<rgb_pixel>> vFaces;
            bool update_faces_database = false;

            std::vector<RectOut> tmp;
            //  for(size_t i = 0; i < rects.size(); i++)
            // {

            //     if((((float)rects[i].box.height/(float)rects[i].box.width) <= 1.5) &&
            //        ((((float)rects[i].box.width/(float)rects[i].box.height) <= 1.5)))    // if face's rectangle scale isn't balance
            //     {
            //         // std::cout << "Face removed" << std::endl;
            //         tmp.push_back(rects[i]);
            //     }
            // }

            // if(rects.size() != tmp.size())
            //     rects = tmp;
            

            for(size_t i = 0; i < rects.size(); i++)
            {
                if((((float)rects[i].box.height/(float)rects[i].box.width) >= 1.5) ||
                   ((((float)rects[i].box.width/(float)rects[i].box.height) >= 1.5)))    // if face's rectangle scale isn't balance
                {
                    // std::cout << "Face removed" << std::endl;
                    continue;
                }

                if(rects[i].id == _face_in_classID)
                {
                    dlib::rectangle face_rect = cvrect_to_dlibrect(rects[i].box);

                    dlib::full_object_detection shape = _sp(cimg, face_rect);
                    matrix<rgb_pixel> face_chip;
                    chip_details chip = get_face_chip_details(shape,150,0.25);
                    extract_image_chip(cimg, chip, face_chip);

                    // vFaces.push_back(move(face_chip));
                    matrix<float,0,1> face_descriptor = _face_net(face_chip);

                    double _face_threshold = _face_threshold_min; //default 0.6 
                    int face_id = findFaces(face_descriptor, _face_threshold);
                    rects[i].name = _faces_name[face_id];

                    // //send to websocket
                    // std::string msg = ">>> | " + _faces_name[face_id] + " | appeared at: " + getDateTime();
                    // _put_ws_msg->send(std::move(std::string(msg)));

                    // float face_percentage = (1 - _face_threshold) * 100;
                    // std::cout << "face_percentage: " << ((1 - _face_threshold) * 100) << "%" << std::endl;
                    rects[i].confidence = 1 - _face_threshold;
                    // rects[i].id = face_id;
                    tmp.push_back(rects[i]);

                    if((_face_threshold < _face_threshold_insert) && (face_id > 0))
                    {
                        _faces_database.insert(_faces_database.begin() + face_id, face_descriptor);
                        _faces_name.insert(_faces_name.begin() + face_id, _faces_name[face_id]);
                        update_faces_database = true;
                        // cout << "\r\n Inserted face to database (threshold " << _face_threshold << "): " << _faces_name[face_id] << endl;
                    }
                }
            }

            rects = tmp;

            if(update_faces_database)
            {
                serialize(_file_faces_name) << _faces_name;
                serialize(_file_faces_database) << _faces_database;
            }

            // _guests_mutex.lock();
            // // _image.push_back(image);
            // for(size_t i = 0; i < rects.size(); i++)
            // {
            //     _rects.push_back(rects[i]);
            // }
            // _guests_mutex.unlock();

            if(_nb_faces != rects.size()) // if number of faces appeared in frame difference from old frame, then send it to ws
            {
                for(size_t i = 0; i < rects.size(); i++)
                {
                    //send to websocket
                    std::string msg = ">>> | " + rects[i].name + " | appeared at: " + getDateTime();
                    _put_ws_msg->send(std::move(std::string(msg)));
                }

                _nb_faces = rects.size();
                _count_nofaces = 0;
            }
            else
            {
                _count_nofaces++; // if have no faces appeared in 10 times, then reset _nb_faces counter.
                if(_count_nofaces >= 10)
                {   
                    _count_nofaces = 0;
                    _nb_faces = 0;
                }
            }

            return rects.size();
        }

    protected:
        int findFaces(matrix<float,0,1> face, double &face_Threshold)
        {
            int face_id = 0;
            // std::string face_name = _classes[0];

            // double tem_threshold = face_Threshold_max;
            // int tem_face_id = 0;
            bool face_found = false;


            for(; face_Threshold < _face_threshold_max; )
            {
                for (size_t face_count = 1; face_count < _faces_database.size(); ++face_count)
                {
                    // Faces are connected in the graph if they are close enough.  Here we check if
                    // the distance between two face descriptors is less than 0.6, which is the
                    // decision threshold the network was trained to use.  Although you can
                    // certainly use any other threshold you find useful.
                    if (length(_faces_database[face_count] - face) <= face_Threshold)
                    {
                        face_found = true;
                        face_id = face_count;
                        break;
                    }
                }

                if(face_found)
                {
                    // face_id = tem_face_id;
                    // face_Threshold = tem_threshold;
                    break;
                }
                
                face_Threshold += _face_threshold_step;
            }

            return face_id;
        }
        //#########################################################################

        // void faceManager()
        // {
        //     while(1)
        //     {
        //         // std::string msg = ">>> | " + _faces_name[face_id] + " | appeared at: " + getDateTime();
        //         // _put_ws_msg->send(std::move(std::string(msg)));
        //     }
        // }

    private:
        std::vector<matrix<float,0,1>> _faces_database;
        std::vector<std::string> _faces_name;
        shape_predictor _sp;
        anet_type _face_net;

        // float _face_threshold = 0; //default 0.6 
        const float _face_threshold_max = 0.391;
        const float _face_threshold_min = 0;
        const float _face_threshold_step = 0.01;
        const float _face_threshold_insert = 0.2;

        size_t _face_in_classID = 0;

        std::string _file_faces_database = "../model/dlib/faces_database.dat";
        std::string _file_faces_name = "../model/dlib/face_names.txt";

	    std::shared_ptr<sBuffer<std::string>> _put_ws_msg = nullptr;
        int _nb_faces = 0;
        int _count_nofaces = 0;
        
        // std::vector<cv::Mat> _image;
        // std::vector<RectOut> _rects;
        // std::mutex _guests_mutex;
        // std::thread _faces_manager_thread;
};

#endif  //__FACE_RECOG_H__