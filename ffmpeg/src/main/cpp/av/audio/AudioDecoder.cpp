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

#include "AudioDecoder.h"
#include "av/core/AudioUtils.h"
#include <sstream>

namespace FFAV {

static const char *FILTER_BUFFER_SRC_NAME = "0:a";
static const char *FILTER_BUFFER_SINK_NAME = "outa";

AudioDecoder::AudioDecoder() {
    
}

AudioDecoder::~AudioDecoder() {
    if ( audio_decoder ) {
        delete audio_decoder;
    }
    
    if ( filter_graph ) {
        delete filter_graph;
    }
    
    if ( pkt ) {
        av_packet_free(&pkt);
    }
    
    if ( dec_frame ) {
        av_frame_free(&dec_frame);
    }
    
    if ( filt_frame ) {
        av_frame_free(&filt_frame);
    }
    
    if ( buf_src_params ) {
        av_free(buf_src_params);
    }
}

int AudioDecoder::init(
    AVCodecParameters* audio_stream_codecpar,
    AVRational audio_stream_time_base,
    AVSampleFormat output_sample_fmt
) {
    
    int ret = initAudioDecoder(audio_stream_codecpar);
    if ( ret < 0 ) {
        return ret;
    }
    
    buf_src_params = audio_decoder->createBufferSrcParameters(audio_stream_time_base);
    
    this->output_sample_fmt = output_sample_fmt;
    output_sample_rate = buf_src_params->sample_rate;
    output_nb_channels = buf_src_params->ch_layout.nb_channels;
    char ch_layout_desc[64];
    av_channel_layout_describe(&buf_src_params->ch_layout, ch_layout_desc, sizeof(ch_layout_desc)); // get channel layout desc
    output_ch_layout_desc = ch_layout_desc;
    
    ret = resetFilterGraph();
    if ( ret < 0 ) {
        return ret;
    }
    
    pkt = av_packet_alloc();
    dec_frame = av_frame_alloc();
    filt_frame = av_frame_alloc();
    return 0;
}

int AudioDecoder::decode(AVPacket* pkt, AudioDecoder::DecodeFrameCallback callback) {
    if ( is_flushed ) is_flushed = false;
    return AudioUtils::transcode(pkt, audio_decoder, dec_frame, filter_graph, filt_frame, FILTER_BUFFER_SRC_NAME, FILTER_BUFFER_SINK_NAME, callback);
}

void AudioDecoder::flush() {
    if ( !is_flushed ) {
        is_flushed = true;
        audio_decoder->flush();
        resetFilterGraph();
    }
}

AVSampleFormat AudioDecoder::getOutputSampleFormat() {
    return output_sample_fmt;
}

int AudioDecoder::getOutputSampleRate() {
    return output_sample_rate;
}

int AudioDecoder::getOutputChannels() {
    return output_nb_channels;
}

int AudioDecoder::initAudioDecoder(AVCodecParameters* codecpar) {
    int ff_ret = 0;
    audio_decoder = new MediaDecoder();
    ff_ret = audio_decoder->init(codecpar);
    if ( ff_ret < 0 ) {
        return ff_ret;
    }
    return 0;
}

int AudioDecoder::initFilterGraph(
    const char* buf_src_name,
    const char* buf_sink_name,
    AVBufferSrcParameters *buf_src_params, 
    AVSampleFormat out_sample_fmt,
    int out_sample_rate,
    const std::string& out_ch_layout_desc
) {    
    if ( filter_graph ) {
        delete filter_graph;
        filter_graph = nullptr;
    }

    int ff_ret = 0;

    // init filter graph
    filter_graph = new FilterGraph();
    ff_ret = filter_graph->init();
    if ( ff_ret < 0 ) {
        return ff_ret;
    }
    
    // cfg filter graph
    const AVSampleFormat out_sample_fmts[] = { out_sample_fmt, AV_SAMPLE_FMT_NONE };
    const int out_sample_rates[] = { out_sample_rate, -1 };
    
    std::stringstream filter_descr_ss;
    filter_descr_ss << "[" << buf_src_name << "]"
                    << "aformat=sample_fmts=" << av_get_sample_fmt_name(out_sample_fmt) << ":channel_layouts=" << out_ch_layout_desc
                    << "[" << buf_sink_name << "]";
    std::string filter_descr = filter_descr_ss.str();
    
    // add buffersrc [0:a]
    ff_ret = filter_graph->addBufferSourceFilter(buf_src_name, AVMEDIA_TYPE_AUDIO, buf_src_params);
    if ( ff_ret < 0 ) {
        return ff_ret;
    }
    
    // add buffersink [outa]
    ff_ret = filter_graph->addAudioBufferSinkFilter(buf_sink_name, out_sample_rates, out_sample_fmts, out_ch_layout_desc);
    if ( ff_ret < 0 ) {
        return ff_ret;
    }

    ff_ret = filter_graph->parse(filter_descr);
    if ( ff_ret < 0 ) {
        return ff_ret;
    }
    
    ff_ret = filter_graph->configure();
    if ( ff_ret < 0 ) {
        return ff_ret;
    } 
    return 0;
}

int AudioDecoder::resetFilterGraph() {
    return initFilterGraph(FILTER_BUFFER_SRC_NAME, FILTER_BUFFER_SINK_NAME, buf_src_params, output_sample_fmt, output_sample_rate, output_ch_layout_desc);
}

}