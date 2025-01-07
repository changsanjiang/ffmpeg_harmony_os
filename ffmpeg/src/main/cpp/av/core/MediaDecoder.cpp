//
// Created on 2025/1/7.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "MediaDecoder.h"

namespace CoreMedia {
    MediaDecoder::MediaDecoder(const std::string& url): url(url), ctx(nullptr), reader(nullptr), pkt(nullptr) {
    
    }

    MediaDecoder::~MediaDecoder() {
        close();
    }

    int MediaDecoder::open() {
        if ( reader != nullptr ) {
            return AVERROR(EAGAIN);
        }
    
        reader = new MediaReader(url);
        if ( reader == nullptr ) {
            return AVERROR(ENOMEM);
        }
        return reader->open();
    }

    int MediaDecoder::getStreamCount() {
        return reader != nullptr ? reader->getStreamCount() : 0;
    }

    AVStream* _Nullable MediaDecoder::getStream(int stream_index) {
        return reader != nullptr ? reader->getStream(stream_index) : nullptr;
    }

    int MediaDecoder::selectStream(int stream_index) {
        if ( reader == nullptr ) {
            return AVERROR_STREAM_NOT_FOUND;
        }
        
        AVStream *stream = reader->getStream(stream_index);
        if ( stream == nullptr ) {
            return AVERROR_STREAM_NOT_FOUND;
        }
    
        int error = reader->selectStream(stream_index);
        if ( error < 0 ) {
            return error;
        }   
        
        // 获取解码器
        AVCodecParameters *codecpar = stream->codecpar;
        const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
        if ( codec == nullptr ) {
            return AVERROR_DECODER_NOT_FOUND; // 找不到解码器
        }
    
        // 创建解码器上下文
        ctx = avcodec_alloc_context3(codec);
        if ( ctx == nullptr ) {
            return AVERROR(ENOMEM);
        }
    
        // 初始化解码器上下文
        // Copy decoder parameters to decoder context
        error = avcodec_parameters_to_context(ctx, codecpar);
        if ( error < 0 ) {
            return error;
        }    
        
        // 打开解码器
        error = avcodec_open2(ctx, codec, nullptr);
        if ( error < 0 ) {
            return error;
        }
        
        pkt = av_packet_alloc();
        if ( pkt == nullptr ) {
            return AVERROR(ENOMEM);
        }
        return 0;
    }    

    int MediaDecoder::decodeFrame(AVFrame* _Nonnull frame) {
        if ( reader == nullptr || ctx == nullptr ) {
            return AVERROR_STREAM_NOT_FOUND;
        }

        if ( pkt == nullptr ) {
            return AVERROR(ENOMEM);
        }        

        int error = 0;
        while(true) {
            error = avcodec_receive_frame(ctx, frame); // receive decoded frames
            if ( error == AVERROR(EAGAIN) ) {
                error = reader->readFrame(pkt); // read packet
                if ( error < 0 ) {
                    return error;
                }

                error = avcodec_send_packet(ctx, pkt); // send packet to codec
                if ( error < 0 ) {
                    return error;
                }
                av_packet_unref(pkt);
            }
            else {
                return error;
            }
        }
        return 0;
    }

    int MediaDecoder::seek(int64_t timestamp, int flags) {
        if ( reader == nullptr || ctx == nullptr ) {
            return AVERROR_STREAM_NOT_FOUND;
        }
        
        int error = reader->seek(timestamp, flags);
        if ( error < 0 ) {
            return error;
        }

        avcodec_flush_buffers(ctx);
        return 0;
    }

    void MediaDecoder::close() {
        if ( pkt != nullptr ) {
            av_packet_free(&pkt);
        }
    
        if ( ctx != nullptr ) {
            avcodec_free_context(&ctx);
        }
    
        if ( reader != nullptr ) {
            delete reader;
            reader = nullptr;
        }
    }
}