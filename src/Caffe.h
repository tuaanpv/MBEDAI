/**********************************************************************************
*   Created 2018 by Johny Pham
*   tuaanpv@gmail.com
*   Youtube/Tiktok/Instagram: tuaanpv
***********************************************************************************/

#ifndef __CAFFE_H__
#define __CAFFE_H__
#include <opencv2/dnn.hpp>
#include <queue>

#include "Buffer.h"
#include "Detection.h"


class Caffe: public Detection
{
    public:
        Caffe(float conf_threshold = 0.2, float nms_threshold = 0.5)
        {
            _obj_name = "Caffe Detector";
            std::cout << _obj_name << std::endl;
            
            _class_file = "../model/caffe/face/labels.txt";
            _model_file = "../model/caffe/face/res10_300x300_ssd_iter_140000.caffemodel";
            _config_file = "../model/caffe/face/deploy.prototxt";

            _conf_threshold = conf_threshold;    //as high as accuracy
            _nms_threshold = nms_threshold;

            _enable_face_recognization = true;
            _face_recog.inClassID(0);

            // _enable_tracking = true;
            // _tracker_iou_threshold = 0.4;

            _net_type = net_caffe;
            readClassFile();
            // _classes.push_back("face");
            loadModel();
        }

    protected:
        std::vector<int> detect(const cv::Mat &image, std::vector<int> &classIds,
                                        std::vector<float> &confidences,
                                        std::vector<cv::Rect> &boxes)
        {
            cv::Mat img2;

            // cv::resize(image, img2, cv::Size(400, 300));
            auto inputBlob = cv::dnn::blobFromImage(image, 1.0, cv::Size(400, 300), cv::Scalar(104.0, 177.0, 123.0));

            net.setInput(inputBlob);
            auto detection = net.forward("detection_out");
            cv::Mat detectionMat(detection.size[2], detection.size[3], CV_32F, detection.ptr<float>());

            // std::vector<std::vector<int>> boxes;
            // std::cout << "detectionMat.rows: " << detectionMat.rows << std::endl;

            int count = 0;
            std::vector<int> indices;
            for (int i = 0; i < detectionMat.rows; i++) 
            {
                float confidence = detectionMat.at<float>(i, 2);
                int classId = (int)(detectionMat.at<float>(i, 1));
                // std::cout << "so far so good2" << std::endl;

                if (confidence > _conf_threshold) 
                {
                    // std::cout << "so far so good3" << std::endl;
                    float left = detectionMat.at<float>(i, 3) * image.cols;
                    float top = detectionMat.at<float>(i, 4) * image.rows;
                    float right = detectionMat.at<float>(i, 5) * image.cols;
                    float bottom = detectionMat.at<float>(i, 6) * image.rows;
                    
                    float width = right - left + 1; 
                    float height = bottom - top + 1;

                    // if((width <= 1) || (height <= 1)) continue;                   

                    boxes.push_back(cv::Rect(left, top, width, height));
                    confidences.push_back(confidence);
                    classIds.push_back(classId-1);
                    // std::cout << "so far so good4" << std::endl;
                    indices.push_back(count);
                    count++;
                }
            }
            // Non-Max Supression               
            // cv::dnn::NMSBoxes(boxes, confidences, _conf_threshold, _nms_threshold, indices);

            return indices;
        }
};

#endif  //__CAFFE_H__