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
// Created on 2025/2/25.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef FFMPEG_HARMONY_OS_AUDIODECODER_H
#define FFMPEG_HARMONY_OS_AUDIODECODER_H

#include "av/core/AudioFifo.h"
#include "av/core/MediaDecoder.h"
#include "av/core/FilterGraph.h"
#include <string>

namespace FFAV {

class AudioDecoder {

public:
    AudioDecoder();
    ~AudioDecoder();
    
    int init(
        AVCodecParameters* audio_stream_codecpar,
        AVRational audio_stream_time_base,
        AVSampleFormat output_sample_fmt
    );
    
    using DecodeFrameCallback = std::function<int(AVFrame* frame)>;
    int decode(AVPacket* pkt, DecodeFrameCallback callback);
    void flush();
    
    AVSampleFormat getOutputSampleFormat();
    int getOutputSampleRate();
    int getOutputChannels();
    
private:
    AVBufferSrcParameters *buf_src_params;
    AVSampleFormat output_sample_fmt;
    int output_sample_rate;
    int output_nb_channels;
    std::string output_ch_layout_desc;
    
    MediaDecoder* audio_decoder { nullptr };
    FilterGraph* filter_graph { nullptr };
    AVPacket *pkt { nullptr };
    AVFrame *dec_frame { nullptr };
    AVFrame *filt_frame { nullptr };
    
    bool is_flushed { true };
    
    int initAudioDecoder(AVCodecParameters* codecpar);
    int initFilterGraph(
        const char* buf_src_name,
        const char* buf_sink_name,
        AVBufferSrcParameters *buf_src_params, 
        AVSampleFormat out_sample_fmt,
        int out_sample_rate,
        const std::string& out_ch_layout_desc
    );
    int resetFilterGraph();
};

}

#endif //FFMPEG_HARMONY_OS_AUDIODECODER_H
