/**********************************************************************************
*   Created 2018 by Johny Pham
*   tuaanpv@gmail.com
*   Youtube/Tiktok/Instagram: tuaanpv
***********************************************************************************/

#ifndef __BUFFER_H__
#define __BUFFER_H__

#include <mutex>
#include <thread>
#include <deque>
#include <vector>
#include <condition_variable>
#include <iterator>
#include <map>

#include <opencv2/dnn.hpp>
#include <opencv2/tracking.hpp>
#include <opencv2/tracking/tracking_legacy.hpp>

#define MAX_VBUFFER_SIZE 10


struct RectOut
{
    int id;
    float confidence;
    cv::Rect box;
    std::string name;
    std::string type;
};

typedef std::map<size_t, std::vector<RectOut>> MapRect;


struct FrameInfo
{
    cv::Mat frame;
    size_t timestamp;
    // int width;
    // int height;
    // float fps;
};

struct VideoInfo
{
    int fps;
    int duration;
    int width;
    int height;
    std::string codec = ".h265"; //".h264"
};

template <typename T>
class vBuffer
{
  public:
    vBuffer(size_t max_size = MAX_VBUFFER_SIZE)
    {
        _max_size = max_size;
    }

    T receive(size_t at = 0)
    {
        T msg;
        std::unique_lock<std::mutex> ulock(_mutex);
        // _cond.wait(ulock, [this]{ return at < _messages.size(); });
        // T msg = std::move(_messages.front());
        // _messages.pop_front();
        at -= _frame_offset;
        if(at < _messages.size())
            // msg = std::move(_messages[at]);
            msg = _messages[at];
        // _messages.erase(_messages.begin() + at);

        return msg;
    }

    T pop()
    {
        T msg;
        std::unique_lock<std::mutex> ulock(_mutex);
        // _cond.wait(ulock, [this]{ return at < _messages.size(); });
        // T msg = std::move(_messages.front());
        // _messages.pop_front();
        if(_messages.size())
        {
            msg = _messages[0];
            _messages.erase(_messages.begin() + 0);
            _frame_offset++;
        }

        return msg;
    }

    void send(T &&msg)
    {
        while(getSize() >= _max_size)
        {
            // T msg = receive();
            std::this_thread::sleep_for(std::chrono::milliseconds(_duration));
        }

        std::unique_lock<std::mutex> ulock(_mutex);
        _messages.push_back(std::move(msg));
        // _cond.notify_one();
    }

    void update(size_t at, T &&msg)
    {
        std::unique_lock<std::mutex> ulock(_mutex);
        at -= _frame_offset;
        if(at < _messages.size())
        {
            _messages[at] = std::move(msg);
        }
    }

    size_t getSize()
    {
        std::unique_lock<std::mutex> ulock(_mutex);
        // std::cout << "Buffer size: " << _messages.size() << "   \r" << std::flush;
        // std::cout << "\rBuffer size: " << _messages.size() << "   ";  // dump progress
        // printf("\rBuffer size: %02d ", _messages.size());
        return _messages.size();
    }

    size_t getSizeOffset()
    {
        std::unique_lock<std::mutex> ulock(_mutex);
        // std::cout << "\rBuffer size: " << _messages.size() << "   ";
        // printf("Buffer size: %02u ", _messages.size());
        return _messages.size() + _frame_offset;
    }

    size_t getPopCounter()
    {
        std::unique_lock<std::mutex> ulock(_mutex);
        return _frame_offset;
    }

    void setFPS(float fps)
    {
        std::unique_lock<std::mutex> ulock(_mutex);
        if(fps > 0)
        {
            _fps = fps;
            _vid_info.fps = fps;
            if(_fps) _duration = 1000/_fps;
        }
    }

    int getFPS()
    {
        std::unique_lock<std::mutex> ulock(_mutex);
        return _fps;
    }

    void setCodec(std::string codec)
    {
        _vid_info.codec = codec;
    }

    std::string getCodec()
    {
        return _vid_info.codec;
    }

    void setVidInfo(VideoInfo vid_info)
    {
        std::unique_lock<std::mutex> ulock(_mutex);
        _vid_info = vid_info;

        if(_vid_info.fps > 0) _fps = _vid_info.fps;
        if(_vid_info.height > 0) _height = _vid_info.height;
        if(_vid_info.width > 0) _width = _vid_info.width;
    }

    VideoInfo getVidInfo()
    {
        std::unique_lock<std::mutex> ulock(_mutex);
        return _vid_info;
    }

    void setClassesNumber(int num)
    {
        std::unique_lock<std::mutex> ulock(_mutex);
        if(num > _classes_number)
            _classes_number = num;
    }

    int getClassesNumber()
    {
        std::unique_lock<std::mutex> ulock(_mutex);
        return _classes_number;
    }

    bool isUpdating()
    {
        std::unique_lock<std::mutex> ulock(_mutex);
        return _continue_update;
    }

    void setUpdating(bool is_update)
    {
        std::unique_lock<std::mutex> ulock(_mutex);
        _continue_update = is_update;
    }

  private:
    std::mutex _mutex;
    // std::condition_variable _cond;
    std::vector<T> _messages;
    // size_t _total = 100;
    int _fps = 1;
    int _duration = 33;
    int _width;
    int _height;
    VideoInfo _vid_info;

    int _classes_number = 100;
    size_t _frame_offset = 0;

    bool _continue_update = true;

    size_t _max_size = MAX_VBUFFER_SIZE;
};


template <typename T>
class mBuffer
{
    public:
        mBuffer(size_t max_size = 1000)
        {
            _max_size = max_size;
        }

        std::vector<T> receive(size_t at = 0)
        {
            std::vector<T> msg;
            std::unique_lock<std::mutex> ulock(_mutex);
            typename MapRect::iterator it = _messages.find(at);
            if(it != _messages.end())
            {
                msg = it->second;
            }

            return msg;
        }

        std::vector<T> pop(size_t at = 0)
        {
            std::vector<T> msg;
            std::unique_lock<std::mutex> ulock(_mutex);
            typename MapRect::iterator it = _messages.find(at);
            if(it != _messages.end())
            {
                msg = it->second;
                _messages.erase(at);
            }

            return msg;
        }

        void send(size_t at, std::vector<T> msg)
        {
            while(getSize() > _max_size)
            {
                pop();
            }

            std::unique_lock<std::mutex> ulock(_mutex);

            typename MapRect::iterator it = _messages.find(at);
            if(it != _messages.end())
            {
                for(size_t i = 0; i < msg.size(); i++)
                    it->second.push_back(msg[i]);
            }
            else
            {
                _messages.insert(make_pair(at, msg));
            }
        }

        size_t getSize()
        {
            return _messages.size();
        }

        void setDataReady(std::thread::id id, size_t status)
        {
            std::unique_lock<std::mutex> ulock(_mutex);

            // std::map<std::thread::id, size_t>::iterator it = _data_ready.find(id);
            _data_ready[id] = status;
            // if(it != _data_ready.end())
            // {
            //     it->second = status;
            // }
            // else
            // {
            //     _data_ready[id] = status;
            // }
        }

        bool getDataReady()
        {
            size_t status = 0;
            std::unique_lock<std::mutex> ulock(_mutex);

            // status = _data_ready.begin()->second;

            for (auto it = _data_ready.begin(); it != _data_ready.end(); ++it)
            {
                // status = it->second;
                // std::cout << "Data ready >>>>>>>>> (thread id: " << it->first << ") = " << it->second << std::endl;
                // status = std::min(status, it->second);
                if(it->second)  status++;
            }

            // std::cout << "status: " << status << std::endl;
            return (status == _data_ready.size())? true:false;
        }

        size_t getDetectingAtFrame(std::thread::id id = 0)
        {
            size_t status = 0;
            std::unique_lock<std::mutex> ulock(_mutex);
            
            if(!(bool)id)
                status = _data_ready[id];
            else
            {
                size_t min_data = 999999;
                for (auto it = _data_ready.begin(); it != _data_ready.end(); ++it)
                {
                    // status = it->second;
                    // std::cout << "Data ready >>>>>>>>> (thread id: " << it->first << ") = " << it->second << std::endl;
                    min_data = std::min(min_data, it->second);
                    // if(it->second < min_data)  min_data = it->second ;
                }
                status = min_data;
            }

            return status;
        }

        size_t getNumDetector()
        {
            size_t num_of_detector = 0;
            std::unique_lock<std::mutex> ulock(_mutex);
            num_of_detector = _data_ready.size();
            return num_of_detector;
        }

    private:
        std::mutex _mutex;
        // std::condition_variable _cond;
        MapRect _messages;
        std::map<std::thread::id, size_t> _data_ready;

        size_t _max_size = 10000;

};


template <typename T>
class sBuffer
{
    public:
        sBuffer(size_t max_size = 1000)
        {
            _max_size = max_size;
        }

        T receive(size_t at = 0)
        {
            T msg;

            std::unique_lock<std::mutex> ulock(_mutex);
            if(_messages.size() > at)   msg = _messages[at];

            return msg;
        }

        T pop(size_t at = 0)
        {
            T msg;

            std::unique_lock<std::mutex> ulock(_mutex);
            if(_messages.size() > at)
            {
                msg = _messages[at];
                _messages.erase(_messages.begin() + at);
            }

            return msg;
        }

        void send(T msg)
        {
            while(getSize() > _max_size)
            {
                pop();
            }

            std::unique_lock<std::mutex> ulock(_mutex);
            _messages.push_back(msg);
        }

        size_t getSize()
        {
            return _messages.size();
        }

    private:
        std::mutex _mutex;
        // std::condition_variable _cond;
        std::vector<T> _messages;

        size_t _max_size = 10000;

};


struct TrackingData
{
    size_t from;
    RectOut rect;
    // dlib::correlation_tracker dlib_tracker;
    cv::Ptr<cv::legacy::Tracker> tracker;
};

#endif  //__BUFFER_H__