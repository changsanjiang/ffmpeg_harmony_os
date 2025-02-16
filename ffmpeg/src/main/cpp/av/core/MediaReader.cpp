/**
    This file is part of @sj/ffmpeg.
    
    @sj/ffmpeg is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    
    @sj/ffmpeg is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with @sj/ffmpeg. If not, see <http://www.gnu.org/licenses/>.
 * */
//
// Created on 2025/1/7.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "MediaReader.h"

namespace FFAV {

static int interrupt_cb(void* ctx) {
    std::atomic<bool>* interrupt_requested = static_cast<std::atomic<bool>*>(ctx); 
    bool shouldInterrupt = interrupt_requested->load(); // 是否请求中断
    return shouldInterrupt ? 1 : 0; // 1 中断, 0 继续;
}

MediaReader::MediaReader() = default;

MediaReader::~MediaReader() { release(); }

int MediaReader::open(const std::string& url) {
    std::lock_guard<std::mutex> lock(interruption_mutex);        
    
    fmt_ctx = avformat_alloc_context();
    if ( fmt_ctx == nullptr ) {
        return AVERROR(ENOMEM);
    }

    fmt_ctx->interrupt_callback = { interrupt_cb, &interrupt_requested };

    int ret = avformat_open_input(&fmt_ctx, url.c_str(), nullptr, nullptr);
    if ( ret < 0 ) {
        return ret;
    }

    return avformat_find_stream_info(fmt_ctx, nullptr);
}

int MediaReader::getStreamCount() { 
    if ( fmt_ctx == nullptr ) {
        return 0;
    }

    return fmt_ctx->nb_streams; 
}

AVStream *_Nullable MediaReader::getStream(int stream_index) {
    if ( fmt_ctx == nullptr ) {
        return nullptr;
    }

    if ( stream_index < 0 || stream_index >= fmt_ctx->nb_streams ) {
        return nullptr;
    }
    
    return fmt_ctx->streams[stream_index];
}

AVStream* _Nullable MediaReader::getBestStream(AVMediaType type) {
    return getStream(findBestStream(type));
 }

int MediaReader::findBestStream(AVMediaType type) {
    if ( fmt_ctx == nullptr ) {
        return AVERROR_INVALIDDATA;
    }

    return av_find_best_stream(fmt_ctx, type, -1, -1, nullptr, 0);
}

int MediaReader::readPacket(AVPacket* _Nonnull pkt) {
    if ( fmt_ctx == nullptr ) {
        return AVERROR_INVALIDDATA;
    }

    std::lock_guard<std::mutex> lock(interruption_mutex);        // 读取前锁定
    return av_read_frame(fmt_ctx, pkt);
}

int MediaReader::seek(int64_t timestamp, int stream_index, int flags) {
    if ( fmt_ctx == nullptr ) {
        return AVERROR_INVALIDDATA;
    }
    
    std::lock_guard<std::mutex> lock(interruption_mutex);        
    return av_seek_frame(fmt_ctx, stream_index, timestamp, flags);
}

void MediaReader::interrupt() {
    interrupt_requested.store(true);
    std::lock_guard<std::mutex> lock(interruption_mutex);        // 等待读取中断        
    interrupt_requested.store(false);
}

void MediaReader::release() {
    interrupt();

    if ( fmt_ctx != nullptr ) {
        avformat_close_input(&fmt_ctx);
    }
}

}