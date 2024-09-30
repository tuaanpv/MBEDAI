/**********************************************************************************
*   Created 2018 by Johny Pham
*   tuaanpv@gmail.com
*   Youtube/Tiktok/Instagram: tuaanpv
***********************************************************************************/

#ifndef __MASK_RCNN_H__
#define __MASK_RCNN_H__
#include <opencv2/dnn.hpp>
#include <queue>
#include <opencv2/dnn.hpp>
#include <opencv2/core/utils/filesystem.hpp>

#include "Buffer.h"
#include "Detection.h"


using namespace std;
using namespace cv;


class MaskRCNN: public Detection
{
    public:
        MaskRCNN(float conf_threshold = 0.3, float nms_threshold = 0.5)
        {
            _obj_name = "Mask R-CNN Detector";
            std::cout << _obj_name << std::endl;

            _class_file = "../model/rcnn/object_detection_classes_coco.txt";
            _model_file = "../model/rcnn/mask_rcnn_inception_v2_coco_2018_01_28.pb";
            _config_file = "../model/rcnn/mask_rcnn_inception_v2_coco_2018_01_28.pbtxt";;

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
            std::vector<int> indices;
            // if (image.empty()) return indices;

            Mat blob;

            // crear el blob 4D a partir de la imagen de entrada
            dnn::blobFromImage(image, blob, 1.0, sz, Scalar(), true, false);
            // Input the blob to the network
            net.setInput(blob);

            // Get the output blob from the network by feed forward
            // std::vector<std::string> outNames = net.getUnconnectedOutLayersNames();
            std::vector<std::string> outNames = { "detection_out_final" , "detection_masks" };
            std::vector<cv::Mat> outs;
            net.forward(outs, outNames);

            Mat preBox = outs[0];  // guarda las regiones rectangulares encontradas
            Mat masks = outs[1];  // guarda las mascaras correspondientes a las regiones 

            const int numDetections = preBox.size[2]; // cantidad de regiones 
            const int num_classes = masks.size[1];    // cantidad de mascaras

            const int frameW = image.cols;
            const int frameH = image.rows;

            for (int i = 0; i < numDetections; ++i) {

                // obtener datos de cada una de las regiones encontradas por la red, se organizan de la siguiente manera:
                // [batchId, classId, confidence, left, top, right, bottom] - 1x1xNx7
                float* box = (float*)preBox.ptr<float>(0, 0, i);
                float score = box[2];

                // nos quedamos con aquellas que superen el umbral establecido
                if (score > _conf_threshold) {

                    int classId   = static_cast<int>(box[1]);
                    int boxLeft   = static_cast<int>(frameW * box[3]);
                    int boxTop    = static_cast<int>(frameH * box[4]);
                    int boxRight  = static_cast<int>(frameW * box[5]);
                    int boxBottom = static_cast<int>(frameH * box[6]);

                    float width = boxRight - boxLeft + 1; 
                    float height = boxBottom - boxTop + 1;

                    // if((width <= 1) || (height <= 1)) continue;

                    // convertimos los datos a uno de tipo cv::Rect 
                    cv::Rect rect{ cv::Point{ boxLeft, boxTop }, cv::Point{ boxRight, boxBottom } };
                    rect &= cv::Rect({ 0,0 }, image.size());

                    // guardamos los datos que nos interesen para su uso posterior
                    classIds.emplace_back(classId);
                    boxes.emplace_back(rect);
                    confidences.emplace_back(score);
                }
            }

            // Non-Max Supression               
            cv::dnn::NMSBoxes(boxes, confidences, _conf_threshold, _nms_threshold, indices);

            return indices;
        }
};

#endif  //__MASK_RCNN_H__