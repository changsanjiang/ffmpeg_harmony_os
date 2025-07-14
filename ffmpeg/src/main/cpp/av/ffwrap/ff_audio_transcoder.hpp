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
//  AudioTranscoder.h
//  LWZFFmpegLib
//
//  Created by sj on 2025/5/16.
//

#ifndef FFAV_AudioTranscoder_hpp
#define FFAV_AudioTranscoder_hpp

#include "ff_types.hpp"
#include "ff_const.hpp"
#include "ff_stream_provider.hpp"

namespace FFAV {
/** 用于将传入的数据包转码为指定格式的pcm数据; */
class AudioTranscoder {
    
public:
    AudioTranscoder() = default;
    virtual ~AudioTranscoder() = default;
    
    /// 初始化
    /// 选择音频流进行转码
    virtual int init(StreamProvider*_Nonnull stream_provider, int output_sample_rate, AVSampleFormat output_sample_format, int output_channels) = 0;
    
    /// pkt 加入转码队列;
    /// eof 时 packet 请传递 nullptr;
    virtual int enqueue(AVPacket *_Nullable packet) = 0;
    virtual int flush(FlushMode mode = FlushMode::Full) = 0;
    
    /// 尝试转码出指定数量的音频数据并读取到 out_data 中;
    ///
    /// 只有数据量足够或者eof时才进行读取操作;
    /// - 返回值 == frame_capacity 表示读取成功;
    /// - 返回值 <= frame_capacity 且 out_eof = true 表示结束;
    /// - 返回值 == 0 表示数据不足，不进行读取操作;
    /// - 出错时返回 ff_err;
    virtual int tryTranscode(void *_Nonnull*_Nonnull out_data, int frame_capacity, int64_t *_Nullable out_pts, bool *_Nullable out_eof) = 0;

    /// 数据包缓冲是否已超过阈值
    virtual bool isPacketBufferFull() = 0;
    
    /// 获取pkt缓冲的endPts; in output time base;
    virtual int64_t getPacketQueueEndPts() = 0;
    /// 获取fifo缓冲的endPts; in output time base;
    virtual int64_t getFifoEndPts() = 0;
    
    /// 获取总时长; in output time base;
    virtual int64_t getDuration() = 0;
    virtual int getOutputSampleRate() = 0;
    virtual AVSampleFormat getOutputSampleFormat() = 0;
    virtual int getOutputChannels() = 0;
};

}

#endif /* FFAV_AudioTranscoder_hpp */
