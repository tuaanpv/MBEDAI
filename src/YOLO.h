/**********************************************************************************
*   Created 2018 by Johny Pham
*   tuaanpv@gmail.com
*   Youtube/Tiktok/Instagram: tuaanpv
***********************************************************************************/

#ifndef __YOLO_H__
#define __YOLO_H__
#include <opencv2/dnn.hpp>
#include <queue>

#include "Buffer.h"
#include "Detection.h"


class YOLO: public Detection
{
    public:
        YOLO(float conf_threshold = 0.3, float nms_threshold = 0.5)
        {
            _obj_name = "[YOLO] ";
            std::cout << _obj_name << std::endl;

            _class_file = "../model/yolo/coco.names";
            _model_file = "../model/yolo/yolov3-tiny.weights";
            _config_file = "../model/yolo/yolov3-tiny.cfg";

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
            cv::Mat frame;
            cv::cvtColor(image, frame, cv::COLOR_BGR2RGB);
            cv::Size siz = letterbox(image, 416);

            // Make a blob of (n, c, h, w)
            cv::Mat blob;
            int inpWidth = 416;  // Width of network's input image
            cv::dnn::blobFromImage(frame, blob, 1/255.0, cv::Size(inpWidth, siz.height), cv::Scalar(0,0,0), true, false);
            // Input the blob to the network
            net.setInput(blob);

            std::vector<cv::Mat> outs;
            net.forward(outs, getOutputsNames(net));
            
            for (size_t i = 0; i < outs.size(); ++i)
            {
                // Scan through all the bounding boxes output from the network and keep only the
                // ones with high confidence scores. Assign the box's class label as the class
                // with the highest score for the box.
                float* data = (float*)outs[i].data;
                for (int j = 0; j < outs[i].rows; ++j, data += outs[i].cols)
                {
                    cv::Mat scores = outs[i].row(j).colRange(5, outs[i].cols);
                    cv::Point classIdPoint;
                    double confidence;
                    // Get the value and location of the maximum score
                    minMaxLoc(scores, 0, &confidence, 0, &classIdPoint);
                    if (confidence > _conf_threshold)
                    {
                        int centerX = (int)(data[0] * frame.cols);
                        int centerY = (int)(data[1] * frame.rows);
                        int width = (int)(data[2] * frame.cols);
                        int height = (int)(data[3] * frame.rows);
                        int left = centerX - width / 2;
                        int top = centerY - height / 2;

                        // if((width <= 1) || (height <= 1)) continue;   
                        
                        classIds.push_back(classIdPoint.x);
                        confidences.push_back((float)confidence); 
                        boxes.push_back(cv::Rect(left, top, width, height));
                    }
                }
            }

            std::vector<int> indices;
            // Non-Max Supression               
            cv::dnn::NMSBoxes(boxes, confidences, _conf_threshold, _nms_threshold, indices);

            return indices;
        }


        // Get the names of the output layers
        std::vector<cv::String> getOutputsNames(const cv::dnn::Net& net)
        {
            static std::vector<cv::String> names;
            if (names.empty())
            {
                //Get the indices of the output layers, i.e. the layers with unconnected outputs
                std::vector<int> outLayers = net.getUnconnectedOutLayers();
                
                //get the names of all the layers in the network
                std::vector<cv::String> layersNames = net.getLayerNames();
                
                // Get the names of the output layers in names
                names.resize(outLayers.size());
                for (size_t i = 0; i < outLayers.size(); ++i)
                names[i] = layersNames[outLayers[i] - 1];
            }
            return names;
        }


        cv::Size letterbox(cv::Mat frame, int new_shape) 
        {
            // Mat frame = inputframe.clone();
            int width = frame.cols;
            int height = frame.rows;
            // cout<<width << " hain? " << height<< endl;
            float ratio = float(new_shape)/std::max(width, height);
            float ratiow = ratio;
            float ratioh = ratio;
            // cout<<width*ratio << " " << height*ratio << endl;
            int new_unpad0 = int(round(width*ratio));
            int new_unpad1 = int(round(height * ratio));
            int dw = ((new_shape - new_unpad0) % 32 )/2;
            int dh = ((new_shape - new_unpad1) % 32 )/2;
            int top = int(round(dh - 0.1));
            int bottom = int(round(dh+0.1));
            int left = int(round(dw - 0.1));
            int right = int(round(dw + 0.1));

            // cout<<" ---- "<< new_unpad0 <<  " " << new_unpad1<<endl;
            cv::resize(frame, frame, cv::Size(new_unpad0, new_unpad1), 0, 0, 1); //CV_INTER_LINEAR = 1
            cv::Scalar value(127.5, 127.5, 127.5);
            cv::copyMakeBorder(frame, frame, top, bottom, left, right, cv::BORDER_CONSTANT, value);
            return frame.size();    
        }
};

#endif  //__YOLO_H__