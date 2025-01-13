//
// Created on 2025/1/7.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef FFMPEGPROJ_MEDIADECODER_H
#define FFMPEGPROJ_MEDIADECODER_H

#include <string>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/packet.h>
#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
#include <libavfilter/buffersrc.h>
}

namespace CoreMedia {
    /** 用于解码 */
    class MediaDecoder {
public:
        MediaDecoder();
        ~MediaDecoder();
    
        int prepare(AVStream* _Nonnull stream);
    
        int send(AVPacket* _Nullable pkt);
        
        int receive(AVFrame* _Nonnull frame);
    
        void flush();
    
        // 生成 buffersrc filter 的构建参数;
        AVBufferSrcParameters* _Nullable createBufferSrcParameters();
    
private:
        AVCodecContext* _Nullable dec_ctx;      // AVCodecContext 用于解码
        AVRational time_base;
        int stream_index;
    
        // 关闭媒体文件
        void release();
    };
}

#endif //FFMPEGPROJ_MEDIADECODER_H
