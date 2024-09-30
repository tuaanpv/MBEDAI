/**********************************************************************************
*   Created 2018 by Johny Pham
*   tuaanpv@gmail.com
*   Youtube/Tiktok/Instagram: tuaanpv
***********************************************************************************/

#ifndef __SVM_H__
#define __SVM_H__
#include <opencv2/dnn.hpp>
#include <queue>

#include <dlib/opencv.h>
#include <opencv2/highgui/highgui.hpp>
#include <dlib/opencv/cv_image.h>
#include "opencv2/imgproc/imgproc.hpp"
#include <opencv2/videoio.hpp>
#include "opencv2/opencv.hpp"
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
#include <dlib/matrix.h>
#include <dlib/sparse_vector.h>
#include <dlib/geometry.h>

#include <stdio.h>
#include <string>
#include <unistd.h>
#include <ctime>
#include <thread>
#include <mutex>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <sys/types.h>
#include <dirent.h>

using namespace std;
using namespace dlib;

#include "Buffer.h"
#include "Detection.h"


typedef scan_fhog_pyramid<pyramid_down<6> > t_image_scanner;
typedef std::vector<object_detector<t_image_scanner> > t_object_detector;


class SVM: public Detection
{
    public:
        SVM(float conf_threshold = 0.0, float nms_threshold = 0.5)
        {
            _obj_name = "[SVM Detector] ";
            std::cout << _obj_name << std::endl;
            
            _conf_threshold = conf_threshold;    //as high as accuracy
            _nms_threshold = nms_threshold;

            loadModel("Stop", "../model/svm/bus_stop.svm");
            loadModel("Parking", "../model/svm/parking.svm");
            loadModel("Restrict", "../model/svm/restrict.svm");

            // _enable_tracking = true;
        }

    protected:
        std::vector<int> detect(const cv::Mat &image, std::vector<int> &classIds,
                                        std::vector<float> &confidences,
                                        std::vector<cv::Rect> &boxes)
        {
            std::vector<int> indices;
            dlib::matrix<dlib::rgb_pixel> cimg = dlib_image(image);

            std::vector<rect_detection> rects;
            evaluate_detectors(detectors, cimg, rects, _conf_threshold);

            int count = 0;
            for(size_t i = 0; i < rects.size(); i++)
            {
                classIds.push_back(rects[i].weight_index);
                confidences.push_back(rects[i].detection_confidence);
                boxes.push_back(dlibrect_to_cvrect(rects[i].rect));
                indices.push_back(count);
                ++count;
            }

            return indices;
        }


    private:
        t_object_detector detectors;
        
        void loadModel(std::string name, std::string model_path)
        {
            object_detector<t_image_scanner> detector;
            deserialize(model_path) >> detector;
            detectors.push_back(detector);
            _classes.push_back(name);
        }
};

#endif  //__SVM_H__