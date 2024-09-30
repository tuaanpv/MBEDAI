/**********************************************************************************
*   Created 2018 by Johny Pham
*   tuaanpv@gmail.com
*   Youtube/Tiktok/Instagram: tuaanpv
***********************************************************************************/

#ifndef __SSDMOBILE_H__
#define __SSDMOBILE_H__
#include <opencv2/dnn.hpp>
#include <queue>

#include "Buffer.h"
#include "Detection.h"


class SSDMobile: public Detection
{
    public:
        SSDMobile(float conf_threshold = 0.47, float nms_threshold = 0.5)
        {
            _obj_name = "[SSD Mobilenet] ";
            std::cout << _obj_name << std::endl;
            
            _class_file = "../model/ssd/ojb/object_detection_classes_coco.txt";
            _model_file = "../model/ssd/ojb/frozen_inference_graph.pb";
            _config_file = "../model/ssd/ojb/ssd_mobilenet_v2_coco_2018_03_29.pbtxt";

            _conf_threshold = conf_threshold;    //as high as accuracy
            _nms_threshold = nms_threshold;

            // _enable_tracking = true;
            
            readClassFile();
            loadModel();
        }

    protected:
        std::vector<int> detect(const cv::Mat &image, std::vector<int> &classIds,
                                        std::vector<float> &confidences,
                                        std::vector<cv::Rect> &boxes)
        {
            // Make a blob of (n, c, h, w)
            cv::Mat blob = cv::dnn::blobFromImage(image, 1.0, sz, cv::Scalar(), swapRB, false);
            // Input the blob to the network
            net.setInput(blob, "", scale, mean);

            // Get the output blob from the network by feed forward
            std::vector<std::string> outNames = net.getUnconnectedOutLayersNames();
            std::vector<cv::Mat> outs;
            net.forward(outs, outNames);

            // The shape of output blob is 1x1xNx7, where N is a number of SSDMobiles and 
            // 7 is a vector of each SSDMobile: 
            //  [batchId, classId, confidence, left, top, right, bottom]
            for(size_t k = 0; k < outs.size(); k++)
            {
                float *data = (float *)outs[k].data;
                for(size_t i = 0; i < outs[k].total(); i += 7)
                {
                    float confidence = data[i+2];
                    int classId = (int)data[i+1];
                    if(confidence > _conf_threshold)
                    {
                        float left = data[i+3] * image.cols;
                        float top = data[i+4] * image.rows;
                        float right = data[i+5] * image.cols;
                        float bottom = data[i+6] * image.rows;

                        // Add 1 because cv::Rect() defines the boundary as left and top are inclusive,
                        //  and as right and bottom are exclusive?
                        float width = right - left + 1; 
                        float height = bottom - top + 1;

                        // if((width <= 1) || (height <= 1)) continue;

                        classIds.push_back(classId - 1); // classID=0 is background, and we have to start
                                                            // the index from 1 as 0 to get a corresponding
                                                            // class name from the class list.
                        confidences.push_back(confidence);
                        boxes.push_back(cv::Rect(left, top, width, height));
                    }            
                }
            }
            std::vector<int> indices;
            // Non-Max Supression               
            cv::dnn::NMSBoxes(boxes, confidences, _conf_threshold, _nms_threshold, indices);

            return indices;
        }
};

#endif  //__SSDMOBILE_H__