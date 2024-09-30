/**********************************************************************************
*   Created 2018 by Johny Pham
*   tuaanpv@gmail.com
*   Youtube/Tiktok/Instagram: tuaanpv
***********************************************************************************/

#ifndef __FFMPEG_LIB_H__
#define __FFMPEG_LIB_H__

/*
 * Convert from OpenCV _image and write movie with FFmpeg
 *
 * Copyright (c) 2016 yohhoy
 */
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <memory>

// FFmpeg
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

// OpenCV
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/core.hpp>

#include "utils.h"

// #include "xop/RtspServer.h"
// #include "net/Timer.h"
#include "Buffer.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>


class FFMPEGLib
{
    public:
        // FFMPEGLib()
        // {

        // }

        // ~FFMPEGLib()
        // {

        // }

        int encode_frame()
        {
        //  av_log_set_level(AV_LOG_DEBUG);
            int ret;

            const int dst_width = _width;
            const int dst_height = _height;
            const AVRational dst_fps = {_fps, 1};

            cv::Mat image;

            // open output format context
            AVFormatContext* outctx = nullptr;
            ret = avformat_alloc_output_context2(&outctx, nullptr, nullptr, _outfile.c_str());
            if (ret < 0) {
                std::cerr << "fail to avformat_alloc_output_context2(" << _outfile << "): ret=" << ret;
                return -1;
            }

            // open output IO context
            // ret = avio_open2(&outctx->pb, _outfile.c_str(), AVIO_FLAG_WRITE, nullptr, nullptr);
            // if (ret < 0) {
            //     std::cerr << "fail to avio_open2: ret=" << ret;
            //     return -1;
            // }

            // create new video stream
            // AVCodec* vcodec = avcodec_find_encoder(AV_CODEC_ID_HEVC);
            const AVCodec* vcodec = avcodec_find_encoder(outctx->oformat->video_codec);
            AVStream* vstrm = avformat_new_stream(outctx, vcodec);
            if (!vstrm) {
                std::cerr << "fail to avformat_new_stream";
                return -1;
            }

            // open video encoder
            AVCodecContext* cctx = avcodec_alloc_context3(vcodec);
            if (!vstrm) {
                std::cerr << "fail to avcodec_alloc_context3";
                return 2;
            }
            //---------------------------------------------------------------------------

            if(_codec == ".h265")
            {
                cctx->codec_id = AV_CODEC_ID_H265;
                cctx->profile = FF_PROFILE_HEVC_MAIN;
                cctx->codec_type = AVMEDIA_TYPE_VIDEO;
                cctx->width = dst_width; // it's 480
                cctx->height = dst_height; // it's 256
                cctx->bit_rate = 384 * 1024;
                cctx->pix_fmt = AVPixelFormat::AV_PIX_FMT_YUV420P;
                cctx->time_base = (AVRational){1, _fps};
                cctx->framerate = (AVRational){_fps, 1};

                AVDictionary* options = NULL;
                av_dict_set(&options, "preset", "ultrafast", 0);
                av_dict_set(&options, "tune", "zero-latency", 0);
                av_opt_set(cctx->priv_data, "x265-params", "keyint=1:crf=18", 0);  // crf: Quality-controlled variable bitrate
                // av_opt_set(cctx->priv_data, "x265-params", "hdr-opt=1:repeat-headers=1:colorprim=bt2020:transfer=smpte2084:colormatrix=bt2020nc:master-display=G(8500,39850)B(6550,2300)R(35400,14600)WP(15635,16450)L(40000000,50):max-cll=0,0", AV_OPT_SEARCH_CHILDREN);

                avcodec_open2(cctx, vcodec, &options);
            }
            else
            {
                cctx->width = dst_width;
                cctx->height = dst_height;
                cctx->pix_fmt = vcodec->pix_fmts[0];
                cctx->time_base = av_inv_q(dst_fps);
                cctx->framerate = dst_fps;
                if (outctx->oformat->flags & AVFMT_GLOBALHEADER)
                    cctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

                ret = avcodec_open2(cctx, vcodec, nullptr);
                if (ret < 0) {
                    std::cerr << "fail to avcodec_open2: ret=" << ret;
                    return 2;
                }
            }

            avcodec_parameters_from_context(vstrm->codecpar, cctx);

            // initialize sample scaler
            SwsContext* swsctx = sws_getContext(
                dst_width, dst_height, AV_PIX_FMT_BGR24,
                dst_width, dst_height, cctx->pix_fmt,
                SWS_BILINEAR, nullptr, nullptr, nullptr);
            if (!swsctx) {
                std::cerr << "fail to sws_getContext";
                return 2;
            }

            //-------------------------------------------------------------------------------

            // allocate frame buffer for encoding
            AVFrame* frame = av_frame_alloc();
            frame->width = dst_width;
            frame->height = dst_height;
            frame->format = static_cast<int>(cctx->pix_fmt);
            ret = av_frame_get_buffer(frame, 32);
            if (ret < 0) {
                std::cerr << "fail to av_frame_get_buffer: ret=" << ret;
                return 2;
            }

            // allocate packet to retrive encoded frame
            AVPacket* pkt = av_packet_alloc();

            // open output IO context
            // ret = avio_open2(&outctx->pb, outfile, AVIO_FLAG_WRITE, nullptr, nullptr);
            // if (ret < 0) {
            //     std::cerr << "fail to avio_open2: ret=" << ret;
            //     return 2;
            // }

            std::cout
                // << "camera:  " << cv_width << 'x' << cv_height << '@' << cv_fps << "\n"
                << "outfile: " << _outfile << "\n"
                << "format:  " << outctx->oformat->name << "\n"
                << "vcodec:  " << vcodec->name << "\n"
                << "size:    " << dst_width << 'x' << dst_height << "\n"
                << "fps:     " << av_q2d(cctx->framerate) << "\n"
                << "pixfmt:  " << av_get_pix_fmt_name(cctx->pix_fmt) << "\n"
                << std::flush;

            // write media container header (if any)
            // ret = avformat_write_header(outctx, nullptr);
            // if (ret < 0) {
            //     std::cerr << "fail to avformat_write_header: ret=" << ret;
            //     return 2;
            // }

            // encoding loop
            // avformat_write_header(outctx, nullptr);
            int64_t frame_pts = 0;
            unsigned nb_frames = 0;
            bool end_of_stream = false;

            for (;;) {
                if (!end_of_stream) 
                {
                    if(_image_out_queue->getSize())
                    {
                        FrameInfo current_frame = _image_out_queue->pop();
                        // fwrite(image.frame.data, 1, (size_t)_width*_height*3, _pipeout);
                        image = current_frame.frame;
                        // count++;
                    }
                }
                
                if(image.empty()) continue;

                if (!end_of_stream) {
                    // convert cv::Mat(OpenCV) to AVFrame(FFmpeg)
                    const int stride[4] = { static_cast<int>(image.step[0]) };
                    sws_scale(swsctx, &image.data, stride, 0, image.rows, frame->data, frame->linesize);
                    frame->pts = frame_pts++;
                    // encode video frame
                    ret = avcodec_send_frame(cctx, frame);
                    if (ret < 0) {
                        std::cerr << "fail to avcodec_send_frame: ret=" << ret << "\n";
                        break;
                    }
                }
                while ((ret = avcodec_receive_packet(cctx, pkt)) >= 0) {
                    // rescale packet timestamp
                    pkt->duration = 1;
                    av_packet_rescale_ts(pkt, cctx->time_base, vstrm->time_base);

                    //deep copy pkt instead of shalow copy
                    AVPacket rtsp_pkt(*pkt);
                    av_init_packet(&rtsp_pkt);
                    av_packet_ref(&rtsp_pkt, pkt);
                    _avpacket_out->send(std::move(AVPacket(rtsp_pkt)));

                    writeVideoFile(pkt->data, pkt->size);

                    av_packet_unref(pkt);
                    std::cout << "FFMPEG frames encoded: " << nb_frames << '\r' << std::flush;  // dump progress
                    ++nb_frames;
                }
                if (ret == AVERROR_EOF)
                    break;
                image.release();
                // if(count >= 1000) break;
            } 

            std::cout << nb_frames << " frames encoded" << std::endl;

            // fclose(fp);

            // write trailer and close file
            av_write_trailer(outctx);
            avio_close(outctx->pb);

            av_packet_free(&pkt);
            av_frame_free(&frame);
            sws_freeContext(swsctx);
            avcodec_free_context(&cctx);
            avformat_free_context(outctx);
            return 0;
        }

        void encodeVideo()
        {
            while(1)
            {
                encode_frame();
            }
        }

        void writeVideoFile(uint8_t* buff, int size)    // write data to file
        {
            static int count_write = 0;

            if(_video_file == nullptr)  // open file if it is not opened
            {
                _outfile = "../result/video_" + getTimeStamp() + _codec;
                std::cout << "Output file at: " << _outfile << endl;

                // std::string command = "echo " + _outfile + " >> ../result/list_output_file.txt";
                // system(command.c_str());


            DIR* dir = opendir("../result");
            if (dir) {
                /* Directory exists. */
                closedir(dir);
            } else if (ENOENT == errno)
            {
                if (mkdir("../result", 0777) == -1)
                    cerr << "Error :  " << strerror(errno) << endl;
            
                else
                    cout << "Directory ../result created";
            } else {
                /* opendir() failed for some other reason. */
            }

                _video_file = fopen(_outfile.c_str(), "wb");
            }

            if(_header_file == nullptr)
            {

                if(_codec == ".h264")
                    _header_size = 736; // h264
                else if(_codec == ".h265")
                    _header_size = 2383; // h265 old 2386

                _header_file = new uint8_t[_header_size]; // h264
                memcpy(_header_file, buff, _header_size); // copy and store header of file

                // std::cout << "------------------- Header data -------------------" << std::endl;
                // for(int i = 0; i < size; i++)
                // {
                //     printf("0x%02x ", buff[i]);
                // }
                // std::cout << "------------------------------- -------------------" << std::endl;
            }
            else
            {
                fwrite(_header_file, 1, _header_size, _video_file); //write header of file
            }

            if(_video_file != nullptr)  // write if file is opened
            {
                fwrite(buff, 1, size, _video_file);
            }

            count_write++;
            if(count_write > 2000)  // close file after 2000 packet to separate the output file
            {
                fclose(_video_file);
                _video_file = nullptr;
                count_write = 0;
            }
        }

		void thread_for_encode()
		{
			if(!_encode_thread.joinable())
				_encode_thread = std::thread(&FFMPEGLib::encodeVideo, this);
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
            _outfile += _codec;
		}

        void setAVPacketQueue(std::shared_ptr<vBuffer<AVPacket>> avpacket_data)
        {
            _avpacket_out = avpacket_data;
        }

    private:
        std::string _codec = ".h265"; //".h264"
        std::string _outfile = "video";
        FILE* _video_file = nullptr;

        uint8_t* _header_file = nullptr;
        int _header_size = 0;

		int _fps;
		int _duration;
		int _width;
		int _height;
    	VideoInfo _vid_info;

		std::thread _encode_thread;
    	std::string _obj_name = "[FFMPEG_LIB] ";

    	std::shared_ptr<vBuffer<FrameInfo>> _image_out_queue = nullptr;
        std::shared_ptr<vBuffer<AVPacket>> _avpacket_out = nullptr;
};

#endif