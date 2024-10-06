/**********************************************************************************
*   Created 2018 by Johny Pham
*   tuaanpv@gmail.com
*   Youtube/Tiktok/Instagram: tuaanpv
***********************************************************************************/

#include <iostream>
#include <random>
#include <iomanip>
#include <thread>

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>

#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include "Graphic.h"
#include "utils.h"


// Constructor.
// Parameter: path to the image file, stored to the private attribute
//            number of _classes, used to generate colors for _classes
Graphic::Graphic(std::string _img_path, int start_at_frame) : _image_path(_img_path)
{
	std::cout << _obj_name << " is started" << std::endl;

    // Open the video once, get information, then close the video
    cv::VideoCapture cap(_image_path);
    _fps = cap.get(cv::CAP_PROP_FPS);
    _width = (int)cap.get(cv::CAP_PROP_FRAME_WIDTH);
    _height = (int)cap.get(cv::CAP_PROP_FRAME_HEIGHT);

    _vid_info.fps = _fps;
    _vid_info.width = _width;
    _vid_info.height = _height;

    _window_size = resizedSize(cv::Size(_width, _height));

    cap.set(cv::CAP_PROP_POS_FRAMES, start_at_frame);
    cap.set(cv::CAP_PROP_BUFFERSIZE, 1); //now the opencv buffer just one frame.

    std::cout << "Image file : " << _image_path << std::endl;
    std::cout << "- original width = " << _width << std::endl;
    std::cout << "- original height = " << _height << std::endl;
    std::cout << "- fps = " << _fps << std::endl;

    if(_fps > 25)
    {
        _skip_frame = 5;
        _fps = 25;
    }
    _duration = 1000/_fps;

    cap.release();
}

Graphic::~Graphic()
{
    _read_thread.join();
    _write_thread.join();

    if(_pipeout != NULL)
    {
        fflush(_pipeout);
        pclose(_pipeout);
    }
}

void Graphic::thread_for_read()
{
    if(!_read_thread.joinable())
        _read_thread = std::thread(&Graphic::readImage, this);
    
    _is_running = true;
}

void Graphic::thread_for_write()
{
    // Video writer
    if(_pipeout == NULL)
    {
        _output_filename = "../result/video_" + getTimeStamp() + ".h265";
        // _output_filename = "rtsp://admin:12345@127.0.0.1:554/live";
        std::cout << "Output file at: " << _output_filename << endl;

        std::string ffmpeg_cmd = std::string("ffmpeg -y -f rawvideo -r ") + std::to_string(_fps) +
            " -video_size " + std::to_string(_width) + "x" + std::to_string(_height) +
            " -pixel_format bgr24 -i pipe: -vcodec libx265 -crf 24 -pix_fmt yuv420p " + _output_filename;
        _pipeout = popen(ffmpeg_cmd.c_str(), "w"); 
    }

    if(!_write_thread.joinable())
        _write_thread = std::thread(&Graphic::writeOutVideo, this);
    
    // _is_running = true;
}

bool Graphic::isActive()
{
    return _is_running;
}

cv::Size Graphic::getWindowSize()
{
    return _window_size;
}

void Graphic::setImageQueue(std::shared_ptr<vBuffer<FrameInfo>> image_in_queue, 
                            std::shared_ptr<mBuffer<RectOut>> _rects_out,
                            std::shared_ptr<vBuffer<FrameInfo>> image_out_queue)
{
    _image_in_queue = image_in_queue;
    // _image_in_queue->setFPS(_fps);
    _image_in_queue->setVidInfo(_vid_info);
    _rects_detected = _rects_out;

    setClassColor(_image_in_queue->getClassesNumber());

    _image_out_queue = image_out_queue;
    _image_out_queue->setVidInfo(_vid_info);
}

float Graphic::getFps()
{
    return _fps;
}

void Graphic::drawResult(cv::Mat &image, std::vector<RectOut> rects_out)
{
    for(size_t i = 0; i < rects_out.size(); i++)
    {
        // Box
        cv::Point p1 = cv::Point(rects_out[i].box.x, rects_out[i].box.y);
        cv::Point p2 = cv::Point(rects_out[i].box.x + rects_out[i].box.width, rects_out[i].box.y + rects_out[i].box.height);
        CV_Assert(rects_out[i].id < _class_color.size());
        // if(rects_out[i].id < _class_color.size()) rects_out[i].id = _class_color.size() - 1;
        cv::rectangle(image, p1, p2, _class_color[rects_out[i].id], 2);

        // Label
        std::ostringstream streamObj;
        streamObj << std::fixed << std::setprecision(2) << rects_out[i].confidence*100.0 << " %";
        std::string label = rects_out[i].name  + " : " + streamObj.str();

        int baseLine;
        cv::Size labelSize = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 0.4, 1, &baseLine);

        int top = std::max(rects_out[i].box.y, labelSize.height);
        cv::Point lp1 = cv::Point(rects_out[i].box.x, top - labelSize.height-2);
        cv::Point lp2 = cv::Point(rects_out[i].box.x + labelSize.width, top);
        cv::rectangle(image, lp1, lp2, _class_color[rects_out[i].id], cv::FILLED);
        cv::putText(image, label, cv::Point(rects_out[i].box.x, top-1), cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(), 1);
    }

    cv::Point lp1 = cv::Point(_width - 110, 5);
    cv::Point lp2 = cv::Point(_width - 10, 28);
    cv::rectangle(image, lp1, lp2, _class_color[88], cv::FILLED);
    cv::putText(image, "Johny Pham", cv::Point(_width - 99, 20), cv::FONT_HERSHEY_SIMPLEX, 0.4, _class_color[43], 1);

    // cv::Point lp1 = cv::Point(5, 28);
    // cv::Point lp2 = cv::Point(160, 5);
    // cv::rectangle(image, lp1, lp2, _class_color[88], cv::FILLED);
    cv::putText(image, getDateTime(), cv::Point(5, _height - 5), cv::FONT_HERSHEY_SIMPLEX, 0.4, _class_color[88], 1);
}


// Read frames from the image file(movie), and send it 
//  to _image_in_queue and __image_in_queue
void Graphic::readImage()
{
    cv::VideoCapture cap(this->_image_path);
    
    if(!cap.isOpened())
    {
        CV_Error(cv::Error::StsError, "Image file (" + _image_path + ") cannot open.");
    }
    
    size_t f_count = 0;
    CV_Assert(this->_image_in_queue != nullptr);

    bool first_time = true;

    bool frame_skipped = false;
    while(true)
    {
        clock_t time_start = clock();
        cv::Mat frame;
        cap >> frame;
        if(frame.empty())
        {
            cap.set(cv::CAP_PROP_POS_FRAMES, 0);    // read first frame of file again
            continue;
            // break;
        }

        if(_skip_frame && f_count && !frame_skipped && (f_count % 5 == 0))
        {
            // std::cout << "skipped frame at: " << f_count << std::endl;
            frame_skipped = true;
            std::this_thread::sleep_for(std::chrono::milliseconds(_duration));
            continue;
        }
        else if(_skip_frame && f_count && frame_skipped && (f_count % 5 == 0))
        {
            frame_skipped = false;
        }

        // if(_scaled_ratio > 1.0)
        // {
        //     // std::cout << "_scaled_ratio: " << _scaled_ratio << std::endl;
        //     // cv::resize(frame, frame, cv::Size(), 1.0/_scaled_ratio, 1.0/_scaled_ratio);
        //     cv::resize(frame, frame, _window_size);
        //     // std::cout << "frame: " << frame.rows << " * " << frame.cols << std::endl;
        // }

        FrameInfo current_frame;
        current_frame.frame = frame;
        current_frame.timestamp = f_count;
        _image_in_queue->send(std::move(FrameInfo(current_frame)));

        ++f_count;
        // _image_in_queue->setTotal(f_count);
        while(first_time)
        {
            if(_rects_detected->getDataReady())
            {
                first_time = false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        // if(first_time)
        // {
        //     first_time = false;
        //     std::this_thread::sleep_for(std::chrono::seconds(3));
        // }

        double diff_time = (double)(clock() - time_start)/(CLOCKS_PER_SEC/1000);
        int sleep_in = (int)(_duration - diff_time);
        if(sleep_in > 0)
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_in));
    }

    _is_running = false;
    _image_in_queue->setUpdating(false);
    std::cout << _obj_name << " End of read image at: " << f_count << std::endl;
    // Send the total counts to the queue
}

// Calculate the resized window size.
// This function returns the resized size where:
//   (width=600 or height=600) && (width <= 600 or height <= 600)
cv::Size Graphic::resizedSize(cv::Size orig)
{
    int w = 800;
    int h = orig.height * (float)w/(float)orig.width;
    if(h > 800)
    {
        int h_orig = h;
        h = 800;
        w = w * ((float)h / (float)h_orig);
    }
    _scaled_ratio = (float)w/(float)orig.width;
    // std::cout << "Video scaled ratio: " << (int)_scaled_ratio << std::endl;
    return cv::Size(w, h);
}

float Graphic::scaledSize(cv::Size orig)
{
    int w = 800;
    int h = orig.height * (float)w/(float)orig.width;
    if(h > 800)
    {
        int h_orig = h;
        h = 800;
        w = w * ((float)h / (float)h_orig);
    }
    // return cv::Size(w, h);

    return (float)orig.width/(float)w;
}

// Assign a color to each class
void Graphic::setClassColor(int class_num)
{
    std::mt19937 random_engine(2019); // Use a fixed seed to get same colors always
    std::uniform_int_distribution<int> distribution(0, 255);

    for(int i = 0; i < class_num; ++i)
    {
        cv::Scalar color = cv::Scalar(distribution(random_engine),
                                      distribution(random_engine),
                                      distribution(random_engine));
        _class_color.push_back(color);
    }
}


void Graphic::setOutPutName(std::string name)
{
    _output_filename = name;
}

void Graphic::writeOutVideoFile(cv::Mat image)
{
    if(_pipeout == NULL)
    {
        _output_filename = "../result/video_" + getTimeStamp() + _vid_info.codec;
        // _output_filename = "rtsp://admin:12345@127.0.0.1:554/live";
        std::cout << "Output file at: " << _output_filename << endl;

        std::string ffmpeg_cmd = std::string("ffmpeg -y -f rawvideo -r ") + std::to_string(_fps) +
            " -video_size " + std::to_string(_width) + "x" + std::to_string(_height) +
            " -pixel_format bgr24 -i pipe: -vcodec libx265 -crf 24 -pix_fmt yuv420p " + _output_filename;
        _pipeout = popen(ffmpeg_cmd.c_str(), "w");
    }

    if(_pipeout != NULL)
    {
        fwrite(image.data, 1, (size_t)_width*_height*3, _pipeout);        
    }

    _packet_count++;
    if(_packet_count >= 1000)  // over 1000 times (packages), write to another new file
    {
        _packet_count = 0;
        fflush(_pipeout);
        pclose(_pipeout);
        _pipeout = nullptr;
    }
}

void Graphic::writeOutVideo()
{
    size_t count = 0;
    while(1)
    {
        if((count >= _image_in_queue->getSize()) && !_image_in_queue->isUpdating() && !_image_out_queue->getSize()) break;

        if(_image_out_queue->getSize())
        {
            // FrameInfo current_frame = _image_out_queue->receive(count);
            FrameInfo current_frame = _image_out_queue->pop();
            // fwrite(image.frame.data, 1, (size_t)_width*_height*3, _pipeout);
            cv::Mat current_img = current_frame.frame;
            if(current_img.empty()) continue;

            writeOutVideoFile(current_img);

            count++;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(_duration));
    }
    std::cout << _obj_name << " End of writing at: " << count << std::endl;
}