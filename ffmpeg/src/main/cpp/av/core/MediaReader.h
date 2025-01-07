//
// Created on 2025/1/7.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef FFMPEGPROJ_MEDIAREADER_H
#define FFMPEGPROJ_MEDIAREADER_H

#include <mutex>
#include <string>

extern "C" {
    #include <libavformat/avformat.h>
    #include <libavcodec/packet.h>
    #include <libavcodec/avcodec.h>
}

namespace CoreMedia {
    /** 用于读取未解码的数据包 */
    class MediaReader {
    public:
        MediaReader(const std::string& url);
        ~MediaReader();
    
        // 打开媒体文件
        int open();
        
        // 获取流的数量
        int getStreamCount();
    
        // 获取指定流的 AVStream
        AVStream* _Nullable getStream(int stream_index);
    
        // 选择流进行读取
        int selectStream(int stream_index);
    
        // 读取下一帧
        int readFrame(AVPacket* _Nonnull pkt);
    
        // 跳转
        int seek(int64_t timestamp, int flags = AVSEEK_FLAG_BACKWARD);
    
        // 中断请求
        void interrupt();                       
    
        // 关闭媒体文件
        void close();
    
    private:
        const std::string url;                  // 媒体文件的路径
        AVFormatContext* _Nullable fmt_ctx;     // AVFormatContext 用于管理媒体文件
        int stream_idx;                         // 选中的流的索引
    
        std::atomic<bool> interrupt_requested;  // 请求中断读取
        std::mutex interruption_mutex;          // 用于等待读取中断的互斥锁
    };
};

#endif //FFMPEGPROJ_MEDIAREADER_H
