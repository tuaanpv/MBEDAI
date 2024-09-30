/**********************************************************************************
*   Created 2018 by Johny Pham
*   tuaanpv@gmail.com
*   Youtube/Tiktok/Instagram: tuaanpv
***********************************************************************************/

#ifndef __DLIBFACES_H__
#define __DLIBFACES_H__
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

class DlibFace: public Detection
{
    public:
        DlibFace()
        {
            _obj_name = "Dlib Faces Detector";
            std::cout << _obj_name << std::endl;

            // deserialize("../model/face/shape_predictor_5_face_landmarks.dat") >> sp;
            deserialize("../model/dlib/shape_predictor_68_face_landmarks.dat") >> sp;
            deserialize("../model/dlib/dlib_face_recognition_resnet_model_v1.dat") >> face_net;
            detector = get_frontal_face_detector();

            deserialize("../model/dlib/face_names.txt") >> _classes;
            deserialize("../model/dlib/faces_database.dat") >> faces_database;
            // cout << "\r\n Loaded faces database: " << faces_database.size() << endl;

            if(_classes.size() != faces_database.size())
            {
                cout << "\r\n Faces database error! " << endl;
                cout << "\r\n Loaded names: " << _classes.size() << endl;
                cout << "\r\n Loaded faces database: " << faces_database.size() << endl;
            }
            else 
                num_face_names = _classes.size();

            // _enable_tracking = true;
            // _tracker_iou_threshold = 0.4;
        }

    protected:
        std::vector<int> detect(const cv::Mat &image, std::vector<int> &classIds,
                                        std::vector<float> &confidences,
                                        std::vector<cv::Rect> &boxes)
        {
            std::vector<int> indices;
            std::vector<matrix<rgb_pixel>> vFaces;
            // std::vector<dlib::full_object_detection> shapes;

            dlib::matrix<dlib::rgb_pixel> cimg = dlib_image(image);

            std::vector<dlib::rectangle>  face_rects = detector(cimg);
            // std::cout << "Faces detected: " << face_rects.size() << std::endl;

            for(size_t i =0; i < face_rects.size(); ++i)
            {
                auto shape = sp(cimg, face_rects[i]);
                matrix<rgb_pixel> face_chip;
                chip_details chip = get_face_chip_details(shape,150,0.25);
                extract_image_chip(cimg, chip, face_chip);

                vFaces.push_back(move(face_chip));
                // shapes.push_back(move(shape));
            }

            // win.clear_overlay();
            // win.set_image(img);
            // win.add_overlay(render_face_detections(shapes));
            // win_faces.set_image(tile_images(vFaces));

            // std::cout << "Faces detected: " << vFaces.size() << std::endl;

            if(vFaces.size())
            {
                std::vector<matrix<float,0,1>> face_descriptors = face_net(vFaces);
                int count = 0;

                for(size_t i = 0; i < face_descriptors.size(); i++)
                {
                    double face_Threshold = face_Threshold_min; //default 0.6 
                    int face_id = faces_recognition(face_descriptors[i], face_Threshold);

                    if((face_Threshold < face_Threshold_insert) && (face_id > 0))
                    {
                        faces_database.insert(faces_database.begin() + face_id, face_descriptors[i]);
                        _classes.insert(_classes.begin() + face_id, _classes[face_id]);
                        // cout << "\r\n Inserted face to database (threshold " << face_Threshold << "): " << _classes[face_id] << endl;
                    }

                    classIds.push_back(face_id);
                    boxes.push_back(dlibrect_to_cvrect(face_rects[i]));
                    confidences.push_back((float)(10/face_Threshold));
                    indices.push_back(count);

                    // std::cout << "Face id: " << face_id << std::endl;
                    // std::cout << "Face name: " << _classes[face_id] << std::endl;
                    // std::cout << "confidence: " << (float)(10/face_Threshold) << std::endl;
                    // std::cout << "Faces detected: " << vFaces.size() << std::endl;
                    // std::cout << "count: " << count << std::endl;

                    ++count;
                }
            }

            // std::cout << _obj_name << " Total time: " << (float)(1000*(clock() - start)/CLOCKS_PER_SEC) << std::endl;

            return indices;
        }

        //#########################################################################
        int faces_recognition(matrix<float,0,1> face, double &face_Threshold)
        {
            int face_id = 0;
            // std::string face_name = _classes[0];

            // double tem_threshold = face_Threshold_max;
            // int tem_face_id = 0;
            bool face_found = false;


            for(; face_Threshold < face_Threshold_max; )
            {
                for (size_t face_count = 1; face_count < faces_database.size(); ++face_count)
                {
                    // Faces are connected in the graph if they are close enough.  Here we check if
                    // the distance between two face descriptors is less than 0.6, which is the
                    // decision threshold the network was trained to use.  Although you can
                    // certainly use any other threshold you find useful.
                    if (length(faces_database[face_count] - face) <= face_Threshold)
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
                
                face_Threshold += face_Threshold_step;
            }

            return face_id;
        }
        //#########################################################################

    private:
        frontal_face_detector detector;
        shape_predictor sp;
        anet_type face_net;

        std::vector<matrix<float,0,1>> faces_database;
        // std::vector<std::string> face_names;
        size_t num_face_names;

        // double face_Threshold = 0.1; //default 0.6 
        const double face_Threshold_max = 0.391;
        const double face_Threshold_min = 0.1;
        const double face_Threshold_step = 0.01;
        const double face_Threshold_insert = 0.2;

        // image_window win, win_faces;
};

#endif  //__DLIBFACES_H__