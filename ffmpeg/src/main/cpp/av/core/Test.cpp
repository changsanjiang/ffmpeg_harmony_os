//
// Created on 2025/1/8.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "Test.h"
#include "MediaReader.h"
#include "MediaDecoder.h"
#include "extension/client_print.h"
#include <bits/errno.h>
#include <thread>

namespace CoreMedia {
    void testMediaReader(const std::string& url) {
        client_print_message3("AAAA: [Test] url=%s", url.c_str());
    
        CoreMedia::MediaReader* reader = new CoreMedia::MediaReader(url);
        int ret = reader->prepare();
        client_print_message3("AAAA: [Test] prepared with status: %d", ret);
        
        int nb_streams = reader->getStreamCount();
        client_print_message3("AAAA: [Test] nb_streams: %d", nb_streams);
        
        reader->selectStream(0);
        client_print_message3("AAAA: [Test] select stream at index: 0");
        
        AVPacket *pkt = av_packet_alloc();
        while ( (ret = reader->readFrame(pkt)) >= 0 ) {
            client_print_message3("AAAA: [Test] readFrame with size: %d", pkt->size);
            av_packet_unref(pkt);
        }

        client_print_message3("AAAA: [Test] clear");
        delete reader;
        av_packet_free(&pkt);
    }

    void testMediaDecoder(const std::string& url) {
        client_print_message3("AAAA: [Test] url=%s", url.c_str());
    
        CoreMedia::MediaDecoder* decoder = new CoreMedia::MediaDecoder(url);
        int ret = decoder->prepare();
        client_print_message3("AAAA: [Test] prepared with status: %d", ret);
        
        int nb_streams = decoder->getStreamCount();
        client_print_message3("AAAA: [Test] nb_streams: %d", nb_streams);
        
        for (int i = 0; i < nb_streams ; ++i ) {
            AVStream *stream = decoder->getStream(i);
            if ( stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO ) {
                decoder->selectStream(i);
                client_print_message3("AAAA: [Test] select audio stream at index: i");
                break;            
            }
        }
    
        AVFrame *frame = av_frame_alloc();
        while ( (ret = decoder->decodeFrame(frame)) >= 0 ) {
            client_print_message3("AAAA: [Test] decode frame success: nb_samples=%d, sample_rate=%d, nb_channels=%d, format=%d", frame->nb_samples, frame->sample_rate, frame->ch_layout.nb_channels, frame->format);
            av_frame_unref(frame);
        }

        client_print_message3("AAAA: [Test] clear");
        delete decoder;
        av_frame_free(&frame);
    }

    void testMediaDecoder2(const std::string &url) {
        client_print_message3("AAAA: [Test] url=%s", url.c_str());
    
        CoreMedia::MediaDecoder* decoder = new CoreMedia::MediaDecoder(url);
        int ret = decoder->prepare();
        client_print_message3("AAAA: [Test] prepared with status: %d", ret);
        
        int nb_streams = decoder->getStreamCount();
        client_print_message3("AAAA: [Test] nb_streams: %d", nb_streams);
        
        for (int i = 0; i < nb_streams ; ++i ) {
            AVStream *stream = decoder->getStream(i);
            if ( stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO ) {
                decoder->selectStream(i);
                client_print_message3("AAAA: [Test] select audio stream at index: i");
                break;            
            }
        }
        
        std::mutex mutex;
        bool interrupted = false;
    
        std::thread decode([&] { 
            AVFrame *frame = av_frame_alloc();
            int error = 0;
            while ( true ) {
                client_print_message3("AAAA: [Test] decode frame");
                error = decoder->decodeFrame(frame);
                if ( error < 0 ) {
                    client_print_message3("AAAA: [Test] decode frame error: %d, EXIT=%d", error, error == AVERROR_EXIT);
                    break;
                }
            
                client_print_message3("AAAA: [Test] decode frame success: nb_samples=%d, sample_rate=%d, nb_channels=%d, format=%d", frame->nb_samples, frame->sample_rate, frame->ch_layout.nb_channels, frame->format);
                av_frame_unref(frame);
                
                std::unique_lock<std::mutex> lock(mutex);
                if ( interrupted ) {
                    client_print_message3("AAAA: [Test] interrupted");
                    break;
                }
            }
            av_frame_free(&frame);
        });
    
        std::thread interrupt([&] {
            std::this_thread::sleep_for(std::chrono::seconds(3)); // 延迟 3 秒         
            decoder->interrupt();
            {
                std::unique_lock<std::mutex> lock(mutex);   
                interrupted = true;
            }
        
            std::this_thread::sleep_for(std::chrono::seconds(3)); // 延迟 3 秒         
        
            decoder->seek(0);
        
            AVFrame *frame = av_frame_alloc();
            int error = 0;
            int idx = 0;
            while ( (error = decoder->decodeFrame(frame)) >= 0 ) {
                client_print_message3("AAAA: [Test] 222 decode frame success: nb_samples=%d, sample_rate=%d, nb_channels=%d, format=%d", frame->nb_samples, frame->sample_rate, frame->ch_layout.nb_channels, frame->format);
                av_frame_unref(frame);
                idx += 1;
                if ( idx > 5 ) {
                    client_print_message3("AAAA: [Test] 222 break decode");
                    break;
                }
            }
            client_print_message3("AAAA: [Test] 222 decode frame error: %d, EXIT=%d", error, error == AVERROR_EXIT);
            av_frame_free(&frame);
        });
    
        decode.join();
        interrupt.join();

        client_print_message3("AAAA: [Test] clear");
        delete decoder;
    }

    void test(const std::string& url) {
        testMediaDecoder2(url);
    }
}