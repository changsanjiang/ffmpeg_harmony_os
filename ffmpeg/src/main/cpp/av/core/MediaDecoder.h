//
// Created on 2025/1/7.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef FFMPEGPROJ_MEDIADECODER_H
#define FFMPEGPROJ_MEDIADECODER_H

#include "av/core/MediaReader.h"
#include <string>

extern "C" {
    #include <libavformat/avformat.h>
    #include <libavcodec/packet.h>
    #include <libavcodec/avcodec.h>
    #include <libavutil/frame.h>
}

namespace CoreMedia {
    /** 用于解码 */
    class MediaDecoder {
public:
        MediaDecoder(const std::string& url);
        ~MediaDecoder();
    
        // 打开媒体文件
        int prepare();
        
        // 获取流的数量
        int getStreamCount();
    
        // 获取指定流的 AVStream
        AVStream* _Nullable getStream(int stream_index);
    
        // 获取选中的流的索引, 未选择时返回-1;
        int getSelectedStreamIndex(); 

        // 选择流进行解码
        int selectStream(int stream_index);
        
        // 解码下一帧
        int decodeFrame(AVFrame* _Nonnull frame);
        
        // 跳转
        int seek(int64_t timestamp, int flags = AVSEEK_FLAG_BACKWARD);
        
        // 中断
        // 执行seek操作前, 需要先中断解码操作(interrupt -> seek -> decodeFrame);
        void interrupt();
    
private:
        MediaReader* reader;                    // MediaReader 用于读取未解码的数据包
        AVCodecContext* _Nullable ctx;          // AVCodecContext 用于解码
        AVPacket* _Nullable pkt;
    
        // 关闭媒体文件
        void release();
    };
}

#endif //FFMPEGPROJ_MEDIADECODER_H
