/**********************************************************************************
*   Created 2018 by Johny Pham
*   tuaanpv@gmail.com
*   Youtube/Tiktok/Instagram: tuaanpv
***********************************************************************************/

#ifndef __GRAPHIC_H__
#define __GRAPHIC_H__

// #include "Detection.h"
#include "Buffer.h"


class Graphic
{
  public:
    Graphic(std::string img_file, int start_at_frame = 0);
    ~Graphic();
    void drawResult(cv::Mat &image, const std::vector<RectOut> rects_out);
    cv::Mat getImage();
    void setImageQueue(std::shared_ptr<vBuffer<FrameInfo>> image_in_queue, 
                            std::shared_ptr<mBuffer<RectOut>> _rects_out,
                            std::shared_ptr<vBuffer<FrameInfo>> image_out_queue);
    float getFps();
    cv::Size getWindowSize();
    void thread_for_read();
    bool isActive();


    void thread_for_draw();
    void thread_for_write();
    void setOutPutName(std::string name);
    void writeOutVideoFile(cv::Mat image);
    void writeOutVideo();
    // void startDrawOutput();

  protected:
    // Information about the input video
    std::string _image_path;

    VideoInfo _vid_info;
    int _fps;
    int _duration;
    int _skip_frame = 0;
    float _scaled_ratio = 1.0;
    int _width;
    int _height;
    cv::Size _window_size;
    // thread for reading images
    std::thread _read_thread;
    bool _is_running;
    // pointer for the queue to send images being read
    std::shared_ptr<vBuffer<FrameInfo>> _image_in_queue = nullptr;
    std::shared_ptr<mBuffer<RectOut>> _rects_detected;
    std::shared_ptr<vBuffer<FrameInfo>> _image_out_queue = nullptr;
    // colors being assigned to _classes randomly
    std::vector<cv::Scalar> _class_color;

    std::string _obj_name = "[ReadImage] ";

    void readImage();
    void setClassColor(int class_num);
    float scaledSize(cv::Size orig);
    cv::Size resizedSize(cv::Size orig);
    
    int _packet_count = 0;
    std::thread _write_thread;
    std::string _output_filename = "output.h265";
    FILE* _pipeout = nullptr;

    // bool _writing_is_running = false;
};

#endif  //__GRAPHIC_H__