/**********************************************************************************
*   Created 2018 by Johny Pham
*   tuaanpv@gmail.com
*   Youtube/Tiktok/Instagram: tuaanpv
***********************************************************************************/

#include <iostream>
#include <string>
#include <fstream>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <time.h>
#include <math.h>

#include <opencv2/dnn.hpp>
#include <opencv2/core/utils/filesystem.hpp>

#include "ctrl.h"
#include "utils.h"
#include "Buffer.h"
#include "Detection.h"


// Detection::Detection()
// {
// }

Detection::~Detection()
{
    _detection_thread.join();
    // _tracking_thread.join();
}

void Detection::thread_for_detection()
{
    if(!_detection_thread.joinable())
        _detection_thread = std::thread(&Detection::objectDetection, this);
    
    // if(!_tracking_thread.joinable())
    //     _tracking_thread = std::thread(&Detection::tracking, this);
}

void Detection::setBuffer(std::shared_ptr<vBuffer<FrameInfo>> detect_queue, 
                            std::shared_ptr<mBuffer<RectOut>> rects_out,
                            std::shared_ptr<sBuffer<std::string>> put_ws_msg)
{
    _image_queue = detect_queue;
    _rects_detected = rects_out;
    _put_ws_msg = put_ws_msg;
    _face_recog.setWSQueue(put_ws_msg);

    _image_queue->setClassesNumber(getClassNumber());

    // _detection_thread = std::thread(&Detection::objectDetection, this);
}

// Return the number of _classes
int Detection::getClassNumber()
{
    return _classes.size();
}

// Return if detected objects
bool Detection::dataReady()
{
    _detection_mutex.lock();
    bool is_data_ready = _data_ready;
    // bool is_tracker_data_ready = _tracker_data_ready;
    _detection_mutex.unlock();

    // return is_data_ready & is_tracker_data_ready;
    return is_data_ready;
}

void Detection::setFPS(int fps)
{
    if(fps > 0)
        _duration = 1000/fps;
}


void Detection::useCUDA(int set_cuda)
{
    _opencv_uses_cuda = set_cuda;
}

void Detection::setModel(std::string model_file, std::string config_file, std::string class_file)
{
    _class_file = class_file;
    _model_file = model_file;
    _config_file = config_file;
    
    _classes.clear();
    readClassFile();
    loadModel();
    
}

// Get images from _image_queue, perform detection,
//  and store the result in queues.
void Detection::objectDetection()
{
    int fps = _image_queue->getFPS();
    if(fps > 0) _duration = 1000/fps;
    // std::cout << _obj_name << " Video fps " << fps << " or " << _duration << "(ms)" << std::endl;

    _thread_id = std::this_thread::get_id();

    size_t d_count = 0;
    _rects_detected->setDataReady(_thread_id, d_count);
    // bool is_data_ready = false;

    while(true)
    {
        clock_t time_start = clock();

        if((d_count >= _image_queue->getSizeOffset()) && !_image_queue->isUpdating()) break;
        if(d_count < _image_queue->getPopCounter()) d_count = _image_queue->getPopCounter() + _detected_frames;

        FrameInfo current_frame = _image_queue->receive(d_count);
        cv::Mat current_img = current_frame.frame;
        // std::cout << _obj_name << "d_count: " << d_count << std::endl;
        if(current_img.empty()) continue;

        std::vector<int> classIds;
        std::vector<float> confidences;
        std::vector<cv::Rect> boxes;
        std::vector<RectOut> rect_out;
        
        // Input the image to the model and get the result
        std::vector<int> indices = detect(current_img, classIds, confidences, boxes);

        for(int index : indices)
        {
            RectOut tmp;
            tmp.id = classIds[index];
            tmp.confidence = confidences[index];
            tmp.name = _classes[classIds[index]];
            tmp.box = boxes[index];
            tmp.type = _obj_name;
            rect_out.push_back(tmp);
        }

        double exe_time = (double)(clock() - time_start)/(CLOCKS_PER_SEC/1000);
        // float exe_time = 1000*diff_time;
        // std::cout << _obj_name << " Executed time (ms): " << exe_time << std::endl; 
        int detected_frames = (int)(std::ceil(exe_time/_duration));
        // std::cout << _obj_name << " - Frames to detect: " << detected_frames << std::endl;
        // _detection_mutex.lock();
        _detected_frames = detected_frames;
        // _detection_mutex.unlock();

        if(rect_out.size())
        {
            if(_enable_face_recognization)
            {
                _face_recog.findFaces(current_frame.frame, rect_out);

                // for(size_t i = 0; i < rect_out.size(); i++)    // send person arrived to websocket
                // {
                //     std::string msg = ">>> | " + rect_out[i].name + " | appeared at: " + getDateTime();
                //     _put_ws_msg->send(std::move(std::string(msg)));
                // }
            }
            
            for(size_t i = 0; i < _detected_frames; i++)
            {
                if(_data_ready)
                    _rects_detected->send(current_frame.timestamp + i, rect_out);

                if(_tracking_running)
                    _rects_buff.send(current_frame.timestamp + i, rect_out);
            }
            // _rects_detected->send(current_frame.timestamp, rect_out);
            // _rects_detected->send(current_frame.timestamp+1, rect_out);
            // _rects_buff.send(current_frame.timestamp, rect_out);
            // _rects_buff.send(current_frame.timestamp+1, rect_out);
        }
        
        if(!_data_ready)
        {
            std::cout << _obj_name << " First time initialization spent " << exe_time << "(ms) or " << detected_frames << "(frames)" << std::endl;
            // is_data_ready = true;
            _detection_mutex.lock();
            _data_ready = true;
            _detection_mutex.unlock();
            _rects_detected->setDataReady(_thread_id, 1);
            continue;
        }

        d_count += _detected_frames;
        _frame_counter = d_count;
        // _rects_detected->setDataReady(_thread_id, d_count);
        // ++count;
        // int sleep_in = (int)(_duration - exe_time);
        // // std::cout << _obj_name << "Sleep in (ms): " << sleep_in << std::endl;
        // if(sleep_in > 0)
        //     std::this_thread::sleep_for(std::chrono::milliseconds(sleep_in));
    }
    std::cout << _obj_name << " End of detection at: " << d_count << std::endl;
}

// Read Class file and store the class list to _classes  .
void Detection::readClassFile()
{
    // Open and read class file
    std::ifstream ifs(_class_file.c_str());
    if(!ifs.is_open())
        CV_Error(cv::Error::StsError, "Class File (" + _class_file + ") not found.");

    std::string line;
    while(std::getline(ifs, line))
    {
        _classes.push_back(line);
    }
}

// Load DNN model and store it to the private attribute.
void Detection::loadModel()
{
    if((_net_type == net_default) || (_net_type == net_maskrcnn) || (_net_type == net_ssdmobile))
        net = cv::dnn::readNet(_model_file, _config_file);
    else if(_net_type == net_tensorflow)
        net = cv::dnn::readNetFromTensorflow(_model_file, _config_file);
    else if(_net_type == net_caffe)
        net = cv::dnn::readNetFromCaffe(_config_file, _model_file);

    _opencv_uses_cuda = isCuda();
    
    //std::cout << cv::getBuildInformation();
    if(_opencv_uses_cuda)
    {
        // std::cout << "OpenCV uses CUDA" << std::endl;
        net.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
        net.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA);
    }
    else
    {
        // std::cout << "OpenCV don't use CUDA" << std::endl;
        net.setPreferableBackend(cv::dnn::DNN_BACKEND_DEFAULT);
        net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
    }
}


void Detection::tracking()
{
    do
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    } while(!_data_ready);

    // std::cout << "_data_ready " << std::endl;
    _tracking_running = true;

    size_t counter = 0;
    while(1)
    {
        clock_t time_start = clock();

        if(_rects_buff.getSize() > 1)
        {
            std::vector<RectOut> rect_now = _rects_buff.receive(counter);
            if(!rect_now.empty())
            {
                // size_t offset = 0;
                for(size_t i = 1; i < 30; i++)
                {
                    std::vector<RectOut> rect_next= _rects_buff.receive(counter + i);
                    if(!rect_next.empty())
                    {
                        if(i < 2)
                        {
                            // counter += i;
                            break;
                        }
                        else
                        {
                            for(size_t j = 1; j < i; j++)
                            {
                                _rects_detected->send(counter + j, rect_now);
                                std::cout << "updated rects at: " << counter + j << std::endl;
                            }
                            // counter += i;
                        }
                        counter += i;
                    }
                }
            }
        }

        double exe_time = (double)(clock() - time_start)/(CLOCKS_PER_SEC/1000);
        int sleep_in = (int)(_duration - exe_time);
        // std::cout << _obj_name << "sleep_in (" << sleep_in << ") = _duration (" << _duration << ") - exe_time(" << exe_time << ")" << std::endl;
        // if(sleep_in) std::this_thread::sleep_for(std::chrono::milliseconds(sleep_in));
        if(sleep_in) usleep(useconds_t(sleep_in * 1000));
        // if(sleep_in) sleep_for_miliseconds(sleep_in);
    }
}

// void Detection::tracking()
// {
//     do
//     {
//         std::this_thread::sleep_for(std::chrono::milliseconds(1));
//     } while(!_data_ready);

//     // std::cout << "_data_ready " << std::endl;

//     std::vector<RectOut> rect_old;
//     size_t counter = _frame_counter;
//     size_t _frame_counter_old = _frame_counter;
//     while(1)
//     {
//         clock_t time_start = clock();

//         if(_frame_counter_old != _frame_counter)
//         {
//             std::cout << _obj_name << "_frame_counter: " << _frame_counter << " | _frame_counter_old: " << _frame_counter_old << " | counter: " << counter << std::endl;
//             counter = _frame_counter;
//             _frame_counter_old = _frame_counter;
//         }

//         std::vector<RectOut> rect_new = _rects_buff.receive(counter);
//         if(rect_new.empty())
//         {  
//             // std::cout << "rect_new.empty() " << std::endl;
//             rect_new = rect_old;
//             _rects_detected->send(counter, rect_new);
//         }

//         // rect_old.clear();
//         rect_old = rect_new;

//         double exe_time = (double)(clock() - time_start)/(CLOCKS_PER_SEC/1000);
//         int sleep_in = (int)(_duration - exe_time);
//         std::cout << _obj_name << "sleep_in (" << sleep_in << ") = _duration (" << _duration << ") - exe_time(" << exe_time << ")" << std::endl;
//         // if(sleep_in) std::this_thread::sleep_for(std::chrono::milliseconds(sleep_in));
//         if(sleep_in) usleep(useconds_t(sleep_in * 1000));
//         // if(sleep_in) sleep_for_miliseconds(sleep_in);
//         counter++;
//     }
// }

