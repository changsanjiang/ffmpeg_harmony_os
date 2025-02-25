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
    AVSampleFormat output_sample_fmt, 
    int output_sample_rate,
    std::string output_ch_layout_desc
) {
    
    this->output_sample_fmt = output_sample_fmt;
    this->output_sample_rate = output_sample_rate;
    this->output_ch_layout_desc = output_ch_layout_desc;
    
    int ret = initAudioDecoder(audio_stream_codecpar);
    if ( ret < 0 ) {
        return ret;
    }
    
    buf_src_params = audio_decoder->createBufferSrcParameters(audio_stream_time_base);
    ret = resetFilterGraph();
    if ( ret < 0 ) {
        return ret;
    }
    
    pkt = av_packet_alloc();
    dec_frame = av_frame_alloc();
    filt_frame = av_frame_alloc();
    return 0;
}

int AudioDecoder::decode(AVPacket* pkt, AudioFifo* fifo, bool should_flush) {
    if ( should_flush ) {
        audio_decoder->flush();
        resetFilterGraph();
        fifo->clear();
    }
    
    return AudioUtils::transcode(pkt, audio_decoder, dec_frame, filter_graph, filt_frame, FILTER_BUFFER_SRC_NAME, FILTER_BUFFER_SINK_NAME, fifo);
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