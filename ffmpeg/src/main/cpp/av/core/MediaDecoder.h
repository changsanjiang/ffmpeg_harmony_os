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

#ifndef FFMPEGPROJ_MEDIADECODER_H
#define FFMPEGPROJ_MEDIADECODER_H

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/packet.h>
#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
#include <libavfilter/buffersrc.h>
#include "libavutil/rational.h"
}

namespace FFAV {

/** 用于解码 */
class MediaDecoder {
public:
    MediaDecoder();
    ~MediaDecoder();

    int init(AVCodecParameters* _Nonnull codecpar);

    int send(AVPacket* _Nullable pkt);
    
    int receive(AVFrame* _Nonnull frame);

    void flush();

    // 生成 buffersrc filter 的构建参数;
    AVBufferSrcParameters* _Nullable createBufferSrcParameters(AVRational time_base);

    AVSampleFormat getSampleFormat();
    int getSampleRate();
    int getChannels();
    
private:
    AVCodecContext* _Nullable dec_ctx;      // AVCodecContext 用于解码

    // 关闭媒体文件
    void release();
};

}

#endif //FFMPEGPROJ_MEDIADECODER_H
