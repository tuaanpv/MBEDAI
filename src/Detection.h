/**********************************************************************************
*   Created 2018 by Johny Pham
*   tuaanpv@gmail.com
*   Youtube/Tiktok/Instagram: tuaanpv
***********************************************************************************/

#ifndef __DETECTION_H__
#define __DETECTION_H__
#include <opencv2/dnn.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "opencv2/imgproc/imgproc.hpp"
#include <opencv2/videoio.hpp>
#include "opencv2/opencv.hpp"
#include <queue>

#include <dlib/opencv.h>
#include <dlib/opencv/cv_image.h>
#include <dlib/image_io.h>
#include <dlib/image_transforms.h>

#include <unistd.h>

#include "utils.h"
#include "ctrl.h"
#include "utils.h"
#include "Buffer.h"
#include "FacesRecog.h"


// static char net_type[] = {"default", "Tensorflow", "Caffe", "MaskRCNN", "SSDMobile"};
enum net_type
{
  net_default = 0,
  net_tensorflow,
  net_caffe,
  net_maskrcnn,
  net_ssdmobile
};

class Detection
{
  public:
    Detection(){};
    ~Detection();
    
    int getClassNumber();
    void thread_for_detection();
    void setBuffer(std::shared_ptr<vBuffer<FrameInfo>> detect_queue, 
                    std::shared_ptr<mBuffer<RectOut>> rects_out,
                    std::shared_ptr<sBuffer<std::string>> put_ws_msg);

    // bool is_data_ready();
    bool dataReady();
    void setFPS(int fps);

    void setModel(std::string model_file = "", std::string config_file = "", std::string class_file = "");

    void tracking();

  protected:
    // Detection threshold
    float _conf_threshold;
    float _nms_threshold;

    // Model, config, label file can be accessed out site of the class
    std::string _class_file;
    std::string _model_file;
    std::string _config_file;

    // Parameters for SSD MobileNet (fixed)
    float scale = 1.f;
    const cv::Scalar mean = cv::Scalar(0, 0, 0);
    const cv::Size sz = cv::Size(300, 300);
    const bool swapRB = true;

    // Store the list of classe name
    std::vector<std::string> _classes;
    // DNN model
    cv::dnn::Net net;
    // Information about detected objects
    std::shared_ptr<vBuffer<FrameInfo>> _image_queue = nullptr;
    std::shared_ptr<mBuffer<RectOut>> _rects_detected = nullptr;
	  std::shared_ptr<sBuffer<std::string>> _put_ws_msg = nullptr;
    // std::condition_variable _cond;
    // thread for detection
    std::thread _detection_thread;
    std::thread::id _thread_id;

    std::mutex _detection_mutex;
    std::string _obj_name = "[Detector] ";
    bool _data_ready = false;
    int _fps = 30;
    float _duration = 1000/_fps;
    size_t _detected_frames = 1; // 1 frame
    size_t _frame_counter = 0;

    void readClassFile();
    void loadModel();
    virtual std::vector<int> detect(const cv::Mat &image, std::vector<int> &classIds,
                                    std::vector<float> &confidences,
                                    std::vector<cv::Rect> &boxes)  = 0;
    void objectDetection();
    // void getResult();

    int _opencv_uses_cuda = 1;
    void useCUDA(int set_cuda);
    net_type _net_type = net_default;

    // std::mutex _tracking_mutex;
    // bool _enable_tracking = false;
    std::thread _tracking_thread;
    mBuffer<RectOut> _rects_buff;
    bool _tracking_running = false;
    // void tracking();
    // mBuffer<RectOut> _rects_tracking;

    bool _enable_face_recognization = false;
    FaceRecog _face_recog;
};

#endif  //__DETECTION_H__