/**********************************************************************************
*   Created 2018 by Johny Pham
*   tuaanpv@gmail.com
*   Youtube/Tiktok/Instagram: tuaanpv
***********************************************************************************/

#ifndef __TENSORFLOW_H__
#define __TENSORFLOW_H__
#include <opencv2/dnn.hpp>
#include <queue>

#include "Buffer.h"
#include "Detection.h"

#include <time.h>

using namespace std;

class Tensorflow: public Detection
{
    public:
        Tensorflow(float conf_threshold = 0.33, float nms_threshold = 0.5)
        {
            _obj_name = "[Tensorflow] ";
            std::cout << _obj_name << std::endl;
            
            _class_file = "../model/ssd/face/classes.txt";
            _model_file = "../model/ssd/face/frozen_inference_graph_face.pb";
            _config_file = "../model/ssd/face/graph.txt";

            _conf_threshold = conf_threshold;    //as high as accuracy
            _nms_threshold = nms_threshold;

            _enable_face_recognization = true;
            _face_recog.inClassID(0);

            // _enable_tracking = true;
            
            _net_type = net_tensorflow;
            readClassFile();
            loadModel();
        }

    protected:
        std::vector<int> detect(const cv::Mat &image, std::vector<int> &classIds,
                                        std::vector<float> &confidences,
                                        std::vector<cv::Rect> &boxes)
        {         
            cv::Mat blob = cv::dnn::blobFromImage(image, 1.0, cv::Size(300, 300), cv::Scalar(), swapRB, false, CV_8U);

            net.setInput(blob);
            // std::cout << "so far so good 1" << std::endl; 

            // Get the output blob from the network by feed forward
            std::vector<std::string> outNames = net.getUnconnectedOutLayersNames();
            std::vector<cv::Mat> outs;
            net.forward(outs, outNames);
            // std::cout << "so far so good 2" << std::endl;

            static std::vector<int> outLayers = net.getUnconnectedOutLayers();
            static std::string outLayerType = net.getLayer(outLayers[0])->type;

            // std::cout << "outs.size(): " << outs.size() << std::endl;
            // std::cout << "outLayerType: " << outLayerType << std::endl;

            if (outLayerType == "DetectionOutput")
            {
                // Network produces output blob with a shape 1x1xNx7 where N is a number of
                // detections and an every detection is a vector of values
                // [batchId, classId, confidence, left, top, right, bottom]
                CV_Assert(outs.size() > 0);
                for (size_t k = 0; k < outs.size(); k++)
                {
                    float* data = (float*)outs[k].data;
                    for (size_t i = 0; i < outs[k].total(); i += 7)
                    {
                        float confidence = data[i + 2];
                        if (confidence > _conf_threshold)
                        {
                            int left   = (int)data[i + 3];
                            int top    = (int)data[i + 4];
                            int right  = (int)data[i + 5];
                            int bottom = (int)data[i + 6];
                            int width  = right - left + 1;
                            int height = bottom - top + 1;
                            if (width <= 2 || height <= 2)
                            {
                                left   = (int)(data[i + 3] * image.cols);
                                top    = (int)(data[i + 4] * image.rows);
                                right  = (int)(data[i + 5] * image.cols);
                                bottom = (int)(data[i + 6] * image.rows);
                                width  = right - left + 1;
                                height = bottom - top + 1;
                            }

                            // if((width <= 1) || (height <= 1)) continue;

                            classIds.push_back((int)(data[i + 1]) - 1);  // Skip 0th background class id.
                            boxes.push_back(cv::Rect(left, top, width, height));
                            confidences.push_back(confidence);
                        }
                    }
                }
            }

            std::vector<int> nmsIndices;
            cv::dnn::NMSBoxes(boxes, confidences, _conf_threshold, _nms_threshold, nmsIndices);
            return nmsIndices;
        }
};

#endif  //__TENSORFLOW_H__