//
// Created on 2025/1/8.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "Test.h"
#include "MediaReader.h"
#include "MediaDecoder.h"
#include "extension/client_print.h"

namespace CoreMedia {
    void test(const std::string& url) {
        testMediaDecoder(url);
    }

    void testMediaReader(const std::string& url) {
        client_print_message3("AAAA: [Test] url=%s", url.c_str());
    
        CoreMedia::MediaReader* reader = new CoreMedia::MediaReader(url);
        int ret = reader->open();
        client_print_message3("AAAA: [Test] open with status: %d", ret);
        
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
        reader->close();
        delete reader;
        av_packet_free(&pkt);
    }

    void testMediaDecoder(const std::string& url) {
        client_print_message3("AAAA: [Test] url=%s", url.c_str());
    
        CoreMedia::MediaDecoder* decoder = new CoreMedia::MediaDecoder(url);
        int ret = decoder->open();
        client_print_message3("AAAA: [Test] open with status: %d", ret);
        
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
        decoder->close();
        delete decoder;
        av_frame_free(&frame);
    }
}