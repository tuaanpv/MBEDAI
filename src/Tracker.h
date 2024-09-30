/**********************************************************************************
*   Created 2018 by Johny Pham
*   tuaanpv@gmail.com
*   Youtube/Tiktok/Instagram: tuaanpv
***********************************************************************************/

#ifndef __TRACKING_H__
#define __TRACKING_H__

#include <opencv2/dnn.hpp>
#include <opencv2/tracking.hpp>
#include <opencv2/tracking/tracking_legacy.hpp>
#include <queue>

#include "Buffer.h"
#include "Detection.h"


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
#include <dlib/matrix.h>
#include <dlib/sparse_vector.h>
#include <dlib/geometry.h>

#include <time.h>
#include "utils.h"

using namespace std;
using namespace dlib;

class Tracking
{
    public:
        Tracking(double min_threshold = 0.4, double max_threshold = 0.77): 
                                        _min_iou_threshold(min_threshold), 
                                        _max_iou_threshold(max_threshold)
        {
            _obj_name = "[Tracking] ";
            std::cout << _obj_name << std::endl;
        }

        ~Tracking()
        {
            if(_tracking_thread.joinable())
                _tracking_thread.join();
        }

        void setBuffer(std::shared_ptr<vBuffer<FrameInfo>> detect_queue, std::shared_ptr<mBuffer<RectOut>> rects_out)
        {
            _image_queue = detect_queue;
            _rects_detected = rects_out;
        }

        void thread_for_tracking()
        {
            if(!_tracking_thread.joinable())
                _tracking_thread = std::thread(&Tracking::objectTracking, this);
        }

    protected:
        void objectTracking()
        {
            float fps = _image_queue->getFPS();
            float duration = 1;
            if(fps > 0) duration = 1000/fps;

            // std::cout << _obj_name << "fps/duration(ms): " << fps << "/" << duration << std::endl;
            
            while(true)
            {
                if(_rects_detected->getDataReady())
                    break;
                
                std::this_thread::sleep_for(std::chrono::seconds(1));
                if(countTimeOut(10))  // time out
                {
                    break;
                }
            }

            std::vector<TrackingData> cv_tracker;

            cv::Mat old_image;

            size_t count = 0;

            while(true)
            {
                // clock_t time_start = clock();
                
                size_t detection_framing = _rects_detected->getDataReady();
                
                if(count >= detection_framing) continue;

                FrameInfo current_frame = _image_queue->receive(count);
                cv::Mat current_img = current_frame.frame;
                if(current_img.empty())
                {
                    count++;
                    continue;
                }

                cv::Mat scoreImg;
                double max_score = 0.77;
                if(!old_image.empty())  // compare scenes of the images are similarly
                {
                    cv::matchTemplate(old_image, current_img, scoreImg, cv::TM_CCOEFF_NORMED);
                    cv::minMaxLoc(scoreImg, 0, &max_score);
                }
                old_image = current_img;

                if(max_score < 0.3) // the scene is difference from previous scene
                {
                    cv_tracker.clear();
                }

                std::vector<RectOut> rects = _rects_detected->receive(current_frame.timestamp);

                for(size_t i = 0; i < rects.size(); i++)
                {
                    // if(rects[i].type != _obj_name) continue;

                    for(size_t j = 0; j < cv_tracker.size(); j++)
                    {
                        if(rects[i].type != cv_tracker[j].rect.type) continue;

                        double overlaping = overlap(rects[i].box, cv_tracker[j].rect.box);

                        if((overlaping >= _min_iou_threshold) && (rects[i].id == cv_tracker[j].rect.id))
                        {
                            cv_tracker.erase(cv_tracker.begin() + j);
                        }
                        else if((overlaping >= _max_iou_threshold) && (rects[i].id != cv_tracker[j].rect.id))
                        {
                            cv_tracker.erase(cv_tracker.begin() + j);
                        }
                    }

                    cv::Ptr<cv::legacy::Tracker> tracker = cv::legacy::TrackerMedianFlow::create(); // best TrackerMedianFlow -> TrackerKCF -> TrackerMOSSE
                    tracker->init(current_img, rects[i].box);
                    TrackingData tmp;
                    tmp.from = count;
                    tmp.rect = rects[i];
                    tmp.tracker = tracker;

                    cv_tracker.push_back(tmp);
                }
                
                for(size_t i = 0; i < cv_tracker.size(); i++)
                {
                    if((count - cv_tracker[i].from) > (int)fps)     // remove if the tracker longer than fps
                    {
                        cv_tracker.erase(cv_tracker.begin() + i);
                        continue;
                    }

                    cv::Rect tmp = cv_tracker[i].rect.box;
                    cv::Point center_point(tmp.width/2 + tmp.tl().x, tmp.height/2 + tmp.tl().y);
                    if((center_point.x < 0) || (center_point.x > current_img.cols) ||
                        (center_point.y < 0) || (center_point.y > current_img.rows))        // remove rectangle if the center point out of image
                    {
                        // std::cout << "Center point: " << center_point.x << " | " << center_point.y << std::endl;
                        cv_tracker.erase(cv_tracker.begin() + i);
                        continue;
                    }

                    if((tmp.width < 10) || (tmp.height < 10))       // remove if the rectangle is so small
                    {
                        cv_tracker.erase(cv_tracker.begin() + i);
                        continue;
                    }
                }

                for(size_t i = 0; i < cv_tracker.size(); i++)
                {
                    if(cv_tracker[i].from < count)
                    {
                        cv::Rect2d rect;
                        cv_tracker[i].tracker->update(current_img, rect);
                        cv_tracker[i].rect.box = static_cast<cv::Rect>(rect);
                    }
                }

                std::vector<RectOut> tracking_rects;
                for(size_t i = 0; i < cv_tracker.size(); i++)
                {
                    tracking_rects.push_back(cv_tracker[i].rect);
                }

                _rects_detected->send(current_frame.timestamp, tracking_rects);

                ++count;

                // int diff_time = (int)(clock() - time_start)/(CLOCKS_PER_SEC/1000);
                // std::cout << _obj_name << "diff_time (ms): " << diff_time << std::endl;
                // int sleep_in = duration - diff_time;
                // std::cout << _obj_name << "sleep_in (ms): " << sleep_in << std::endl;
                // if(sleep_in > 0)
                //     std::this_thread::sleep_for(std::chrono::milliseconds(sleep_in));
            }
        }

    private:
        std::shared_ptr<vBuffer<FrameInfo>> _image_queue = nullptr;
        std::shared_ptr<mBuffer<RectOut>> _rects_detected;

        std::thread _tracking_thread;

        std::string _obj_name;

        double _min_iou_threshold = 0.4F;
        double _max_iou_threshold = 0.77F;
};

#endif  //__TRACKING_H__