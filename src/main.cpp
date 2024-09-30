/**********************************************************************************
*   Created 2018 by Johny Pham
*   tuaanpv@gmail.com
*   Youtube/Tiktok/Instagram: tuaanpv
***********************************************************************************/

#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>

#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include <opencv2/core/utils/filesystem.hpp>
#include <time.h>

#include <cstdlib> // setenv

#if CV_VERSION_MAJOR < 4
#pragma message( "OpenCV version < 4" )
#endif

#include "SSDMobile.h"
#include "Caffe.h"
#include "MaskRCNN.h"
#include "YOLO.h"
#include "YOLOTraffic.h"
#include "SVM.h"
#include "Graphic.h"
// #include "DlibFaces.h"
#include "Tensorflow.h"
#include "Tracker.h"
#include "rtsp_stream.h"
#include "ffmpeg_lib.h"
#include "WebSocket.h"
#include "fileManager.h"

#define _VERSION_MAJOR  1
#define _VERSION_MINOR  0

int main(int argc, char** argv)
{
    std::string img_file;
    int start_at = 0;
    if(argc < 2)
    {
        std::cout << "You must give video source" << std::endl;
        std::cout << argv[0] << " ../images/Nigella.mp4" << std::endl;
        std::cout << argv[0] << "Now open webcam: /dev/video0" << std::endl;
        img_file = "/dev/video0";
        // return 0;
    }
    else if(argc == 2)
    {
        img_file = argv[1];
    }
    else if(argc == 3)
    {
        img_file = argv[1];
        start_at = atoi(argv[2]);
    }

    // Get command line options.

    // const cv::String keys = 
    //     "{help h usage ? |      | print this message. }"
    //     "{c conf         |   .5 | Confidence threshold. }"
    //     "{n nms          |   .5 | Non-max suppression threshold. }"
    //     "{@input         |<none>| Input image or movie file. }";
    // cv::CommandLineParser parser(argc, argv, keys);
    // parser.about("SSD MobileNet Object Detection with C++");
    // if(parser.has("help") || argc == 1)
    // {
    //     parser.printMessage();
    //     return 0;
    // }

    // // Check if the input file has been specified properly.
    // std::string img_file = parser.get<std::string>("@input");
    // if(img_file == "")
    // {
    //     std::cout << "Input file is not specified.\n";
    //     parser.printMessage();
    //     return 0;
    // }
    // if(!cv::utils::fs::exists(img_file))
    // {
    //     std::cout << "Input file (" << img_file << ") not found.\n" ;
    //     return 0;
    // }

    // int using_cuda = isCuda();

    std::string _obj_name = "MainThread ";

    const std::string window_name= "MBEDAI Johny Pham";
    cv::namedWindow(window_name, cv::WINDOW_FREERATIO);

    // Create queues for sending image to display and to detection
    std::shared_ptr<vBuffer<FrameInfo>> image_in_queue(new vBuffer<FrameInfo>);
    std::shared_ptr<mBuffer<RectOut>> rects_out(new mBuffer<RectOut>);
    std::shared_ptr<vBuffer<FrameInfo>> image_out_queue(new vBuffer<FrameInfo>);

    std::shared_ptr<sBuffer<std::string>> put_msg(new sBuffer<std::string>);
    std::shared_ptr<sBuffer<std::string>> get_msg(new sBuffer<std::string>);

    // Create SSD MobileNet model
    // DlibFace face_reg = DlibFace();
    SSDMobile ssd_mobile = SSDMobile(0.7, 0.5);
    Tensorflow tf_face = Tensorflow(0.7, 0.5);
    // YOLOTraffic yolo_traffic_signs = YOLOTraffic(0.8, 0.5);
    // SVM svm = SVM();
    // MaskRCNN mask_rcnn = MaskRCNN();
    // YOLO yolo = YOLO();
    // YOLO yolo_traffic_signs = YOLO(0.5, 0.5);
    // yolo_traffic_signs.setModel("../model/yolo/traffic_signs/yolov4-tiny_training_last.weights", 
    //                             "../model/yolo/traffic_signs/yolov4-tiny_training.cfg", 
    //                             "../model/yolo/traffic_signs/signs.names.txt");
    // Caffe caffe_face = Caffe();
    // Tracking obj_tracker = Tracking(0.2, 0.77);

    // Set shared pointers of queues into objects
    // face_reg.setBuffer(image_in_queue, rects_out);
    ssd_mobile.setBuffer(image_in_queue, rects_out, put_msg);
    tf_face.setBuffer(image_in_queue, rects_out, put_msg);
    // yolo_traffic_signs.setBuffer(image_in_queue, rects_out);
    // svm.setBuffer(image_in_queue, rects_out);
    // mask_rcnn.setBuffer(image_in_queue, rects_out);
    // yolo.setBuffer(image_in_queue, rects_out);
    // caffe_face.setBuffer(image_in_queue, rects_out);
    // obj_tracker.setBuffer(image_in_queue, rects_out);

    Graphic input = Graphic(img_file, start_at);
    input.setImageQueue(image_in_queue, rects_out, image_out_queue);

    // Launch the readinig thread and the detecting thread
    input.thread_for_read();
    ssd_mobile.thread_for_detection();
    tf_face.thread_for_detection();
    // yolo_traffic_signs.thread_for_detection();
    // svm.thread_for_detection();
    // mask_rcnn.thread_for_detection(); // std::this_thread::sleep_for(std::chrono::seconds(5));
    // yolo.thread_for_detection(); 
    // caffe_face.thread_for_detection();
    // obj_tracker.thread_for_tracking();

    cv::resizeWindow(window_name, input.getWindowSize());

    int _fps = (int)input.getFps();
    int duration = (int)(1000/input.getFps());
    // if(_fps <= 25) duration -= 20;
    // else if(_fps > 25) duration = 1;
    
    size_t count = 0;

    // std::this_thread::sleep_for(std::chrono::seconds(5));
    while(true)
    {
        // if(tf_face.dataReady() && ssd_mobile.dataReady() && svm.dataReady())
        if(rects_out->getDataReady())
            break;
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if(countTimeOut(10))  // time out
        {
            break;
        }
    }

    //-------------------------------------------------------------------------------
    // input.thread_for_write();
    std::string _codec = ".h265"; //".h264"
    image_out_queue->setCodec(_codec);

    std::shared_ptr<vBuffer<AVPacket>> _avpacket_out(new vBuffer<AVPacket>);

    FFMPEGLib ffmpeg_codec;
    ffmpeg_codec.setImageQueue(image_out_queue);
    ffmpeg_codec.setAVPacketQueue(_avpacket_out);
    ffmpeg_codec.thread_for_encode();   // encode video, use h265 ffmpeg
    //-------------------------------------------------------------------------------

    //-------------------------------------------------------------------------------
    LiveStream live_stream = LiveStream();
    live_stream.setImageQueue(image_out_queue);
    live_stream.setAVPacketQueue(_avpacket_out);
    // live_stream.setAVFormatContext(ffmpeg_codec.getAVFormatContext());
    // live_stream.thread_for_write();
    live_stream.thread_for_stream();
    //-------------------------------------------------------------------------------

    //-------------------------------------------------------------------------------
    WebSocket web_socket;
    web_socket.setWSQueue(put_msg, get_msg);
    web_socket.thread_for_run();
    //-------------------------------------------------------------------------------

    // std::vector<std::string>* fileNames = new std::vector<std::string>;
    // std::cout << ">>>>>>>>>>>>>>Number of file result: " << listFile("../result/", fileNames) << std::endl;

    // std::cout << _obj_name << " So far so good\n";

    // while(1)
    while(cv::waitKey(duration) < 0)
    {
        // clock_t time_start = clock();

        if(!input.isActive())
        {
            if(!image_in_queue->getSize()) break;
        }
        else
        {
            if(image_in_queue->getSize() < 2) continue;
        }

        FrameInfo current_frame = image_in_queue->pop();
        cv::Mat current_img = current_frame.frame;
        if(current_img.empty()) continue;

        std::vector<RectOut> rect = rects_out->pop(current_frame.timestamp);
        input.drawResult(current_img, rect);

        image_out_queue->send(std::move(FrameInfo(current_frame)));

        cv::imshow(window_name, current_img);
        // image_stream = current_img;
        // live_stream.SendFrameThread(current_img);
        // writer << current_img;

        // input.writeOutVideoFile(current_img);

        ++count;

        // double exe_time = (double)(clock() - time_start)/(CLOCKS_PER_SEC/1000);
        // if(exe_time) std::this_thread::sleep_for(std::chrono::milliseconds((int)exe_time));
        // std::this_thread::sleep_for(std::chrono::milliseconds(duration));
    }
    std::cout << _obj_name << " --- Object detection finished. ---\n";
    // cv::waitKey(10);
    kill(getpid(), SIGKILL);  // itself I think
    return 0;
}

std::string getVersion()
{
    std::string _version = "\nCPP version: " + std::to_string(_VERSION_MAJOR) + '.' + std::to_string(_VERSION_MINOR) + '\n';
    return _version;
}
