//
// Created on 2025/1/7.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "StreamReader.h"

namespace CoreMedia {
    StreamReader::StreamReader(const std::string &url) : url(url), fmt_ctx(nullptr), s_idx(-1) {}
    
    StreamReader::~StreamReader() { close(); }
    
    int StreamReader::open() {
        if (fmt_ctx != nullptr) {
            return AVERROR(EAGAIN); // 已经打开，不需要再次打开
        }
    
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
    
    int StreamReader::getStreamCount() { return fmt_ctx != nullptr ? fmt_ctx->nb_streams : 0; }
    
    AVStream *_Nullable StreamReader::getStream(int stream_index) {
        if (fmt_ctx != nullptr && stream_index >= 0 && stream_index < fmt_ctx->nb_streams) {
            return fmt_ctx->streams[stream_index];
        }
        return nullptr;
    }

    int StreamReader::selectStream(int stream_index) {
        if ( stream_index < 0 || stream_index >= fmt_ctx->nb_streams ) {
            return AVERROR_STREAM_NOT_FOUND;
        }
        s_idx = stream_index;
        return 0;
    }
    
    int StreamReader::readFrame(AVPacket *pkt) {
        if ( fmt_ctx == nullptr || s_idx == -1 ) {
            return AVERROR_STREAM_NOT_FOUND;
        }
    
        int ret = 0;
        while ((ret = av_read_frame(fmt_ctx, pkt)) >= 0) {
            if (pkt->stream_index == s_idx) {
                return 0; // 找到匹配的流
            }
            av_packet_unref(pkt); // 释放不需要的包
        }
        return ret;
    }
    
    int StreamReader::seek(double timestamp, int flags) {
       if ( fmt_ctx == nullptr || s_idx == -1 ) {
            return AVERROR_STREAM_NOT_FOUND;
        }
        return av_seek_frame(fmt_ctx, s_idx, timestamp, flags);
    }
    
    void StreamReader::close() {
        if (fmt_ctx != nullptr) {
            avformat_close_input(&fmt_ctx);
        }
    }
}