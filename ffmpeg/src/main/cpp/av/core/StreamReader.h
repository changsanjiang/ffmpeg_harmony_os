//
// Created on 2025/1/7.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef FFMPEGPROJ_STREAMREADER_H
#define FFMPEGPROJ_STREAMREADER_H

#include <string>

extern "C" {
    #include <libavformat/avformat.h>
    #include <libavcodec/packet.h>
    #include <libavcodec/avcodec.h>
}

namespace CoreMedia {

    class StreamReader {
    public:
        StreamReader(const std::string& url);
        ~StreamReader();
    
        // 打开媒体文件
        int open();
        
        // 获取流的数量
        int getStreamCount();
    
        // 获取指定流的 AVStream
        AVStream* _Nullable getStream(int stream_index);
    
        // 选择流进行读取
        int selectStream(int stream_index);
    
        // 读取下一帧
        int readFrame(AVPacket* pkt);
    
        // 跳转
        int seek(double timestamp, int flags = AVSEEK_FLAG_BACKWARD);
    
        // 关闭媒体文件
        void close();
    
    private:
        const std::string url;                  // 媒体文件的路径
        AVFormatContext* _Nullable fmt_ctx;     // AVFormatContext 用于管理媒体文件
        int s_idx;                       
    };

};

#endif //FFMPEGPROJ_STREAMREADER_H
