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

#ifndef FFMPEGPROJ_MEDIAREADER_H
#define FFMPEGPROJ_MEDIAREADER_H

#include <mutex>
#include <string>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/packet.h>
#include <libavcodec/avcodec.h>
#include <libavcodec/codec.h>
#include <libavutil/avutil.h>
}

namespace FFAV {

/** 用于读取未解码的数据包 */
class MediaReader {
public:
    MediaReader();
    ~MediaReader();

    // 打开媒体文件
    int open(const std::string& url);
    
    // 获取流的数量
    int getStreamCount();

    // 获取指定流的 AVStream
    AVStream* _Nullable getStream(int stream_index);    
    AVStream* _Nullable getBestStream(AVMediaType type);

    /* av_find_best_stream
     *
     * @return  the non-negative stream number in case of success,
     *          AVERROR_STREAM_NOT_FOUND if no stream with the requested type
     *          could be found,
     *          AVERROR_DECODER_NOT_FOUND if streams were found but no decoder
    */
    int findBestStream(AVMediaType type);
    
    // 读取下一帧
    int readPacket(AVPacket* _Nonnull pkt);

    /* 跳转
     *
     * av_seek_frame
     *
     * @param stream_index If stream_index is (-1), a default stream is selected,
     *                     and timestamp is automatically converted from
     *                     AV_TIME_BASE units to the stream specific time_base.
     * @param timestamp    Timestamp in AVStream.time_base units or, if no stream
     *                     is specified, in AV_TIME_BASE units.
     * @param flags        flags which select direction and seeking mode
    */
    int seek(int64_t timestamp, int stream_index, int flags = AVSEEK_FLAG_BACKWARD);

    // 中断读取
    // 执行seek操作前, 需要先中断读取操作(interrupt -> seek -> readFrame);
    void interrupt();

private:
    AVFormatContext* _Nullable fmt_ctx = nullptr;     // AVFormatContext 用于管理媒体文件
    std::atomic<bool> interrupt_requested { false };  // 请求读取中断
    std::mutex interruption_mutex;          // 用于等待读取中断的互斥锁

    // 关闭媒体文件
    void release();
};

}
#endif //FFMPEGPROJ_MEDIAREADER_H
