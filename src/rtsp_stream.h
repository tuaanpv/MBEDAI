/**********************************************************************************
*   Created 2018 by Johny Pham
*   tuaanpv@gmail.com
*   Youtube/Tiktok/Instagram: tuaanpv
***********************************************************************************/

#ifndef __RTSP_STREAM_H__
#define __RTSP_STREAM_H__

// RTSP Server

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libswscale/swscale.h"
#include <libavutil/imgutils.h>
}
#include <opencv2/core.hpp>

#include "xop/RtspServer.h"
#include "net/Timer.h"
#include <thread>
#include <memory>
#include <iostream>
#include <string>

#include "utils.h"
#include "Buffer.h"
#include "ffmpeg_lib.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "popen2.h"

#define AUTH_CONFIG

// xop::MediaSessionId session_id;
// std::shared_ptr<xop::RtspServer> server;

class LiveStream
{
	public:
		~LiveStream()
		{
			_stream_thread.join();
		}
		LiveStream()
		{

		}

		void rtspInit()
		{
			std::cout << _obj_name << " is started" << std::endl;

			// _codec = _image_out_queue->getCodec();

			std::string suffix = "live";
			std::string ip = "127.0.0.1";
			std::string port = "6789";
			std::string rtsp_url = "rtsp://" + ip + ":" + port + "/" + suffix;
			
			event_loop = new xop::EventLoop();
			server = xop::RtspServer::Create(event_loop);

			if (!server->Start("0.0.0.0", atoi(port.c_str()))) {
				printf("RTSP Server listen on %s failed.\n", port.c_str());
				// return 0;
			}
			else
			{
				#ifdef AUTH_CONFIG
					server->SetAuthConfig("-_-", "admin", "12345");
					std::cout << "View with command: ffplay rtsp://admin:12345@127.0.0.1:554/live" << std::endl;
					rtsp_url = "rtsp://admin:12345@" + ip + ":" + port + "/" + suffix;
				#else
					std::cout << "View with command: ffplay rtsp://127.0.0.1:554/live" << std::endl;
				#endif

					session = xop::MediaSession::CreateNew("live"); 
					if(_codec == ".h265")
						session->AddSource(xop::channel_0, xop::H265Source::CreateNew()); 
					else if(_codec == ".h264")
						session->AddSource(xop::channel_0, xop::H264Source::CreateNew()); 

					//session->StartMulticast(); 
					session->AddNotifyConnectedCallback([] (xop::MediaSessionId sessionId, std::string peer_ip, uint16_t peer_port){
						printf("RTSP client connect, ip=%s, port=%hu \n", peer_ip.c_str(), peer_port);
					});
				
					session->AddNotifyDisconnectedCallback([](xop::MediaSessionId sessionId, std::string peer_ip, uint16_t peer_port) {
						printf("RTSP client disconnect, ip=%s, port=%hu \n", peer_ip.c_str(), peer_port);
					});

					// xop::MediaSessionId session_id = server->AddSession(session);
					session_id = server->AddSession(session);
						
					// std::thread t1(SendFrameThread, server.get(), session_id, &h264_file);
					// t1.detach(); 

					std::cout << "Play URL: " << rtsp_url << std::endl;
			}
		}

		void SendFrameThread()
		{
			rtspInit();

			while(1) 
			{
				if(_avpacket_out->getSize())
				{
					// std::cout << "_avpacket_out->getSize(): " << _avpacket_out->getSize() << std::endl;
					AVPacket pkt = _avpacket_out->pop();
					int frame_size = pkt.size;

					if(frame_size > 0) 
					{
						xop::AVFrame videoFrame = {0};
						videoFrame.type = 0; 
						videoFrame.size = frame_size;
						if(_codec == ".h265")
							videoFrame.timestamp = xop::H265Source::GetTimestamp();
						else if(_codec == ".h264")
							videoFrame.timestamp = xop::H264Source::GetTimestamp();
						videoFrame.buffer.reset(new uint8_t[videoFrame.size]);
						memcpy(videoFrame.buffer.get(), pkt.data, videoFrame.size);
						server->PushFrame(session_id, xop::channel_0, videoFrame);
					}
					// av_free_packet(&pkt); // segment coredump
					av_packet_unref(&pkt);
				}
				// xop::Timer::Sleep(_duration);
			}
		}

		void thread_for_stream()
		{
			if(!_stream_thread.joinable())
				_stream_thread = std::thread(&LiveStream::SendFrameThread, this);
			
			// _is_running = true;
		}

		void setImageQueue(std::shared_ptr<vBuffer<FrameInfo>> image_out_queue)
		{
			_image_out_queue = image_out_queue;

			_vid_info = _image_out_queue->getVidInfo();

			_fps = _vid_info.fps;
			_duration = 1000/_fps;
			_height = _vid_info.height;
			_width = _vid_info.width;
			_codec = _vid_info.codec;
		}

        void setAVPacketQueue(std::shared_ptr<vBuffer<AVPacket>> avpacket_data)
        {
            _avpacket_out = avpacket_data;
        }

	private:
    	std::string _obj_name = "[RtspStream] ";
		std::string _codec = ".h265"; //".h264"
		int _fps;
		int _duration;
		int _width;
		int _height;
    	VideoInfo _vid_info;

		xop::EventLoop* event_loop = nullptr;
		std::shared_ptr<xop::RtspServer> server;
		xop::MediaSessionId session_id;
		xop::MediaSession *session = nullptr;
		std::thread _stream_thread;

    	std::shared_ptr<vBuffer<FrameInfo>> _image_out_queue = nullptr;
        std::shared_ptr<vBuffer<AVPacket>>_avpacket_out = nullptr;
};

#endif