/**********************************************************************************
*   Created 2018 by Johny Pham
*   tuaanpv@gmail.com
*   Youtube/Tiktok/Instagram: tuaanpv
***********************************************************************************/

#ifndef __UTILS_H__
#define __UTILS_H__

#include <opencv2/dnn.hpp>
#include <queue>

#include "Buffer.h"
#include "color_text.h"


#include <dlib/opencv.h>
#include <dlib/opencv/cv_image.h>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing/render_face_detections.h>
#include <dlib/svm_threaded.h>
#include <dlib/gui_widgets.h>
#include <dlib/image_processing.h>
#include <dlib/data_io.h>
#include <dlib/image_transforms.h>
#include <dlib/cmd_line_parser.h>
#include <dlib/dir_nav.h>
#include <dlib/clustering.h>
#include <dlib/string.h>
#include <dlib/dnn.h>
#include <dlib/image_io.h>
#include <dlib/matrix.h>
#include <dlib/sparse_vector.h>
#include <dlib/geometry.h>
#include <dlib/queue.h>

#include <time.h>
#include <chrono>
#include <iomanip> // put_time
#include <fstream>
#include <sstream> // stringstream
#include <dirent.h>
#include <errno.h>

// using namespace dlib;
// using namespace std;


static dlib::matrix<dlib::rgb_pixel> dlib_image(cv::Mat image, float scale = 1.0)
{    
    if(scale != 1.0f)
        cv::resize(image, image, cv::Size(), scale, scale);
    // cv::Mat contrast_high;
    // image.convertTo(image, -1, 1, 0); //increase the contrast by 1

    dlib::cv_image<dlib::bgr_pixel> cimg(image);

    dlib::matrix<dlib::rgb_pixel> frame;
    dlib::assign_image(frame, cimg);
    // dlib::pyramid_up(frame);

    return frame;
}

static cv::Rect dlibrect_to_cvrect(dlib::rectangle r)
{
    return cv::Rect(cv::Point2i(r.left(), r.top()), cv::Point2i(r.right(), r.bottom()));
}

static dlib::rectangle cvrect_to_dlibrect(cv::Rect r)
{
    return dlib::rectangle((long)r.tl().x, (long)r.tl().y, (long)r.br().x, (long)r.br().y);
}

// Calculate the resized window size.
// This function returns the resized size where:
//   (width=600 or height=600) && (width <= 600 or height <= 600)
static cv::Size resizedSize(cv::Size orig, int new_size = 800)
{
    int w = new_size;
    int h = orig.height * (float)w/(float)orig.width;
    if(h > new_size)
    {
        int h_orig = h;
        h = new_size;
        w = w * ((float)h / (float)h_orig);
    }
    // std::cout << "Video scaled ratio: " << (int)_scaled_ratio << std::endl;
    return cv::Size(w, h);
}

static float scaledSize(cv::Size orig)
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


static bool overlap(const cv::Rect& lhs, const cv::Rect& rhs, double thres_hold)
{
    double i = static_cast<double>((lhs & rhs).area());
    double u = static_cast<double>((lhs | rhs).area());
    double iou = i / u;
    return iou > thres_hold;
}

static double overlap(const cv::Rect& lhs, const cv::Rect& rhs)
{
    double i = static_cast<double>((lhs & rhs).area());
    double u = static_cast<double>((lhs | rhs).area());
    double iou = i / u;
    return iou;
}

static cv::Rect rectWithoutRect(cv::Rect r1, cv::Rect r2)
{
    // 32 bit integer values:
    int minVal = -2147483648;
    int maxVal =  2147483647;

    // rectangles that define the space left, right, top and bottom of r2
    cv::Rect leftOf  = cv::Rect(cv::Point(minVal, minVal), cv::Point(r2.x, maxVal)); // rect covering the whole area left of r2
    cv::Rect topOf   = cv::Rect(cv::Point(minVal, minVal), cv::Point(maxVal, r2.y)); // rect covering the whole area top of r2
    cv::Rect rightOf = cv::Rect(cv::Point(r2.x+r2.width, minVal), cv::Point(maxVal, maxVal)); // rect covering the whole area left of r2
    cv::Rect bottomOf= cv::Rect(cv::Point(minVal, r2.y+r2.height), cv::Point(maxVal,maxVal)); // rect covering the whole area top of r2

    // intersect the spaces with r1 to find regions of r1 that lie left, right, top and bottom of r2
    cv::Rect allExterior[4];
    allExterior[0] = leftOf;
    allExterior[1] = topOf;
    allExterior[2] = rightOf;
    allExterior[3] = bottomOf;

    // now choose the biggest one
    int biggestSize = 0;
    cv::Rect biggestRect(0,0,0,0);

    for(unsigned int i=0; i<4; ++i)
    {
        cv::Rect intersection = allExterior[i] & r1;
        int size = intersection.width * intersection.height ;
        if(size > biggestSize)
        {
            biggestSize = size;
            biggestRect = intersection;
        }
    }

    return biggestRect;
}

static bool countTimeOut(int count_to)
{
    static int counter = 0;
    counter++;
    if(counter > count_to)
    {
        counter = 0;
        return true;
    }
    else return false;
}



enum e_using_cuda
{
    NOT_CUDA = 0,
    USE_CUDA = 1,
    USE_CUDNN =2
};

static int isCuda()
{
    int using_cuda = 0; // return 0 if CUDA and cuDNN not found
                        // return 0x00000001 if CUDA found
                        // return 0x00000010 if cuDNN found


    // std::cout << cv::getBuildInformation();
    std::string build_info = cv::getBuildInformation();

    size_t found = build_info.find("NVIDIA CUDA:");
    if(found != std::string::npos)
    {
        if((build_info.find("YES", found ) - found) <= 33);
        {
            using_cuda |= 0x01;
            // std::cout << "OpenCV uses NVIDIA CUDA: YES" << std::endl;
        }
    }

    found = build_info.find("cuDNN:");
    if(found != std::string::npos)
    {
        if((build_info.find("YES", found ) - found) <= 33);
        {
            using_cuda |= 0x02;
            // std::cout << "OpenCV uses cuDNN: YES" << std::endl;
        }
    }

    if(using_cuda && USE_CUDA)
    {
        std::cout << "OpenCV uses NVIDIA CUDA: YES" << std::endl;
    }
    if(using_cuda && USE_CUDNN)
    {
        std::cout << "OpenCV uses cuDNN: YES" << std::endl;
    }

    return using_cuda;
}



#define DATE_AND_TIME   0
#define DATE_ONLY   1
#define TIME_ONLY   2
// type = 0: date and time
// type = 1: date only
// type = 2: time only
static std::string getTimeStamp(int type = DATE_AND_TIME)  
{
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);

    std::ostringstream oss;

    if(type == DATE_AND_TIME)
        oss << std::put_time(&tm, "%Y%m%d_%H%M%S");
    else if(type == DATE_ONLY)
        oss << std::put_time(&tm, "%Y%m%d");
    else if(type == TIME_ONLY)
        oss << std::put_time(&tm, "%H%M%S");
    // auto str = oss.str();

    return oss.str();
}

static std::string getDateTime(int type = DATE_AND_TIME)
{
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);

    std::ostringstream oss;

    if(type == DATE_AND_TIME)
        oss << std::put_time(&tm, "%d/%m/%Y %H:%M:%S");
    else if(type == DATE_ONLY)
        oss << std::put_time(&tm, "%d/%m/%Y");
    else if(type == TIME_ONLY)
        oss << std::put_time(&tm, "%H:%M:%S");
    // auto str = oss.str();

    return oss.str();
}

static void sleep_for_miliseconds(int usec)
{
    clock_t time_start = clock();

    while(1)
    {
        usleep(useconds_t(1000));
        double exe_time = (double)(clock() - time_start)/(CLOCKS_PER_SEC/1000);
        if((int)exe_time >= usec) break;
    }
}

// static int matToBytes(cv::Mat image, byte* bytes)
// {
//     int size = image.total() * image.elemSize();
//     // std::cout << "size image.total() * image.elemSize(): " << size << std::endl;

//     if(size)
//     {
//         // if(bytes == NULL)
//         //     bytes = new byte[size];  // you will have to delete[] that later
//         // else
//         // {
//         //     delete bytes;
//         //     bytes = new byte[size];  // you will have to delete[] that later
//         // }
//         bytes = new byte[size];  // you will have to delete[] that later
//         std::memcpy(bytes, image.data, size * sizeof(byte));
//     }

//     return size;
// }

// static cv::Mat bytesToMat(byte * bytes, int width, int height)
// {
//     cv::Mat image = cv::Mat(height, width, CV_8UC3, bytes).clone(); // make a copy
//     return image;
// }

#endif  //__UTILS_H__