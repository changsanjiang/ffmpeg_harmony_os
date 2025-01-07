//
// Created on 2025/1/7.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "MediaReader.h"

namespace CoreMedia {
    static int interrupt_cb(void* ctx) {
        std::atomic<bool>* interrupt_requested = static_cast<std::atomic<bool>*>(ctx); 
        bool shouldInterrupt = interrupt_requested->load(); // 是否请求中断
        return shouldInterrupt ? 1 : 0; // 1 中断, 0 继续;
    }

    MediaReader::MediaReader(const std::string& url) : url(url), fmt_ctx(nullptr), stream_idx(-1), interrupt_requested(false), interruption_mutex() {}
    
    MediaReader::~MediaReader() { close(); }
    
    int MediaReader::open() {
        if (fmt_ctx != nullptr) {
            return AVERROR(EAGAIN); // 已经打开，不需要再次打开
        }
        
        fmt_ctx = avformat_alloc_context();
        if ( fmt_ctx == nullptr ) {
            return AVERROR(ENOMEM);
        }
    
        fmt_ctx->interrupt_callback = { interrupt_cb, &interrupt_requested };
    
        int ret = avformat_open_input(&fmt_ctx, url.c_str(), nullptr, nullptr);
        if (ret < 0) {
            return ret;
        }
    
        ret = avformat_find_stream_info(fmt_ctx, nullptr);
        if (ret < 0) {
            close();
            return ret;
        }
        return 0;
    }
    
    int MediaReader::getStreamCount() { return fmt_ctx != nullptr ? fmt_ctx->nb_streams : 0; }
    
    AVStream *_Nullable MediaReader::getStream(int stream_index) {
        if (fmt_ctx != nullptr && stream_index >= 0 && stream_index < fmt_ctx->nb_streams) {
            return fmt_ctx->streams[stream_index];
        }
        return nullptr;
    }

    int MediaReader::selectStream(int stream_index) {
        if ( fmt_ctx == nullptr || stream_index < 0 || stream_index >= fmt_ctx->nb_streams ) {
            return AVERROR_STREAM_NOT_FOUND;
        }
        stream_idx = stream_index;
        return 0;
    }
    
    // read 操作是阻塞调用
    // 如果想并发操作, 可以开两个线程:
    // 一个线程进行 read 操作, 另一个线程进行 seek 或 close 操作;
    // 在进行 seek 或 close 操作时, 需要判断是否正在进行读取, 需要中断读取然后再进行接下来的操作; 
    
    int MediaReader::readFrame(AVPacket* _Nonnull pkt) {
        if ( fmt_ctx == nullptr || stream_idx == -1 ) {
            return AVERROR_STREAM_NOT_FOUND;
        }
    
        int ret = 0;
        std::lock_guard<std::mutex> lock(interruption_mutex);        // 读取前锁定
        while ((ret = av_read_frame(fmt_ctx, pkt)) >= 0) {
            if (pkt->stream_index == stream_idx) {
                return 0; // 找到匹配的流
            }
            av_packet_unref(pkt); // 释放不需要的包
        }
        return ret;
    }
    
    int MediaReader::seek(int64_t timestamp, int flags) {
        if ( fmt_ctx == nullptr || stream_idx == -1 ) {
            return AVERROR_STREAM_NOT_FOUND;
        }

        interrupt();
        return av_seek_frame(fmt_ctx, stream_idx, timestamp, flags);
    }

    void MediaReader::interrupt() {
        interrupt_requested.store(true);
        std::lock_guard<std::mutex> lock(interruption_mutex);        // 等待读取中断        
        interrupt_requested.store(false);
    }
    
    void MediaReader::close() {
        interrupt();

        if (fmt_ctx != nullptr) {
            avformat_close_input(&fmt_ctx);
        }
    }
}