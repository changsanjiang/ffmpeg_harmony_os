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
// Created on 2025/1/10.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "FilterGraph.h"
#include <sstream>

extern "C" {
#include "libavutil/opt.h"
#include "libavfilter/buffersink.h"
}

namespace FFAV {

FilterGraph::FilterGraph() = default;
FilterGraph::~FilterGraph() { release(); }

int FilterGraph::init() {
    filter_graph = avfilter_graph_alloc();
    if ( filter_graph == nullptr ) {
        return AVERROR(ENOMEM);
    }
    return 0;
}

int FilterGraph::addAudioBufferSourceFilter(const std::string& name, const AVRational time_base, int sample_rate, AVSampleFormat sample_fmt, const AVChannelLayout ch_layout) {
    if ( filter_graph == nullptr ) {
        return AVERROR_INVALIDDATA;
    }
        
    if ( instances[name] != nullptr ) {
        return AVERROR(EAGAIN);
    }

    if ( abuffer == nullptr ) {
        abuffer = avfilter_get_by_name("abuffer");
    }

    if ( abuffer == nullptr ) {
        return AVERROR_FILTER_NOT_FOUND;
    }
    
    // get channel layout desc
    char ch_layout_desc[64];
    av_channel_layout_describe(&ch_layout, ch_layout_desc, sizeof(ch_layout_desc));
    
    std::stringstream src_ss;
    src_ss  << "time_base=" << time_base.num << "/" << time_base.den
            << ":sample_rate=" << sample_rate
            << ":sample_fmt=" << av_get_sample_fmt_name(sample_fmt)
            << ":channel_layout=" << ch_layout_desc;

    AVFilterContext *abuffersrc_ctx = nullptr;
    int ret = avfilter_graph_create_filter(&abuffersrc_ctx, abuffer, name.c_str(), src_ss.str().c_str(), NULL, filter_graph);
    if ( ret < 0 ) {
        return ret;
    }

    return addBufferSourceFilter(name, abuffersrc_ctx);
}

int FilterGraph::addVideoBufferSourceFilter(const std::string& name, const AVRational time_base, int width, int height, AVPixelFormat pix_fmt, const AVRational sar, const AVRational frame_rate) {
    if ( filter_graph == nullptr ) {
        return AVERROR_INVALIDDATA;
    }        

    if ( instances[name] != nullptr ) {
        return AVERROR(EAGAIN);
    }


    if ( vbuffer == nullptr ) {
        vbuffer = avfilter_get_by_name("buffer");
    }

    if ( vbuffer == nullptr ) {
        return AVERROR_FILTER_NOT_FOUND;
    }
    
    std::stringstream src_ss;
    src_ss  << "video_size=" << width << "x" << height
            << ":pix_fmt=" << pix_fmt
            << ":time_base=" << time_base.num << "/" << time_base.den
            << ":pixel_aspect=" << sar.num << "/" << sar.den;
    if ( frame_rate.num ) {
        src_ss << ":frame_rate=" << frame_rate.num << "/" << frame_rate.den;
    }

    AVFilterContext *vbuffersrc_ctx = nullptr;
    int ret = avfilter_graph_create_filter(&vbuffersrc_ctx, vbuffer, name.c_str(), src_ss.str().c_str(), NULL, filter_graph);
    if ( ret < 0 ) {
        return ret;
    }

    return addBufferSourceFilter(name, vbuffersrc_ctx);
}

int FilterGraph::addBufferSourceFilter(const std::string& name, AVMediaType type, const AVBufferSrcParameters* _Nonnull params) {
    if ( instances[name] != nullptr ) {
        return AVERROR(EAGAIN);
    }

    switch(type) {
    case AVMEDIA_TYPE_VIDEO:
        return addVideoBufferSourceFilter(name, params->time_base, params->width, params->height, static_cast<AVPixelFormat>(params->format), params->sample_aspect_ratio, params->frame_rate);
    case AVMEDIA_TYPE_AUDIO:
        return addAudioBufferSourceFilter(name, params->time_base, params->sample_rate, static_cast<AVSampleFormat>(params->format), params->ch_layout);
    case AVMEDIA_TYPE_UNKNOWN:
    case AVMEDIA_TYPE_DATA:
    case AVMEDIA_TYPE_SUBTITLE:
    case AVMEDIA_TYPE_ATTACHMENT:
    case AVMEDIA_TYPE_NB:
        return AVERROR_INVALIDDATA;
    }
}

int FilterGraph::addAudioBufferSinkFilter(const std::string& name, const int* _Nullable sample_rates, const AVSampleFormat* _Nullable sample_fmts, const std::string& channel_layout) {
    if ( instances[name] != nullptr ) {
        return AVERROR(EAGAIN);
    }

    if ( filter_graph == nullptr ) {
        return AVERROR_INVALIDDATA;
    }

    if ( abuffersink == nullptr ) {
        abuffersink = avfilter_get_by_name("abuffersink");
    }
    
    if ( abuffersink == nullptr ) {
        return AVERROR_FILTER_NOT_FOUND;
    }

    AVFilterContext *abuffersink_ctx = nullptr;
    int ret = avfilter_graph_create_filter(&abuffersink_ctx, abuffersink, name.c_str(), NULL, NULL, filter_graph);
    if ( ret < 0 ) {
        return ret;
    }

    ret = av_opt_set_int_list(abuffersink_ctx, "sample_rates", sample_rates, -1, AV_OPT_SEARCH_CHILDREN);
    if ( ret < 0 ) {
        return ret;
    }

    ret = av_opt_set_int_list(abuffersink_ctx, "sample_fmts", sample_fmts, AV_SAMPLE_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
    if ( ret < 0 ) {
        return ret;
    }
    
    ret = av_opt_set(abuffersink_ctx, "ch_layouts", channel_layout.c_str(), AV_OPT_SEARCH_CHILDREN);
    if ( ret < 0 ) {
        return ret;
    }
    return addBufferSinkFilter(name, abuffersink_ctx);
}

int FilterGraph::addVideoBufferSinkFilter(const std::string& name, const AVPixelFormat* _Nullable pix_fmts) {
    if ( instances[name] != nullptr ) {
        return AVERROR(EAGAIN);
    }

    if ( filter_graph == nullptr ) {
        return AVERROR_INVALIDDATA;
    }

    if ( vbuffersink == nullptr ) {
        vbuffersink = avfilter_get_by_name("buffersink");
    }
    
    if ( vbuffersink == nullptr ) {
        return AVERROR_FILTER_NOT_FOUND;
    }

    AVFilterContext *vbuffersink_ctx = nullptr;
    int ret = avfilter_graph_create_filter(&vbuffersink_ctx, vbuffersink, name.c_str(), NULL, NULL, filter_graph);
    if ( ret < 0 ) {
        return ret;
    }
    
    ret = av_opt_set_int_list(vbuffersink_ctx, "pix_fmts", pix_fmts, AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
    if ( ret < 0 ) {
        return ret;
    }

    return addBufferSinkFilter(name, vbuffersink_ctx);    
}

int FilterGraph::addBufferSourceFilter(const std::string& name, AVFilterContext* _Nonnull buffer_ctx) {
    AVFilterInOut *node = avfilter_inout_alloc();
    if ( node == nullptr ) {
        return AVERROR(ENOMEM);
    }

    node->name = av_strdup(name.c_str());
    node->filter_ctx = buffer_ctx;
    node->pad_idx = 0;
    node->next = NULL;
    
    if ( outputs == nullptr ) {
        outputs = node;
    }
    else {
        lastOutput->next = node;
    }
    lastOutput = node;
    instances[name] = buffer_ctx;
    return 0;    
}

int FilterGraph::addBufferSinkFilter(const std::string& name, AVFilterContext* _Nonnull buffersink_ctx) {
    AVFilterInOut *node = avfilter_inout_alloc();
    if ( node == nullptr ) {
        return AVERROR(ENOMEM);
    }

    node->name = av_strdup(name.c_str());
    node->filter_ctx = buffersink_ctx;
    node->pad_idx = 0;
    node->next = NULL;
    
    if ( inputs == nullptr ) {
        inputs = node;
    }
    else {
        lastInput->next = node;
    }
    lastInput = node;
    instances[name] = buffersink_ctx;
    return 0;
}

int FilterGraph::parse(const std::string& filter_descr) {
    int ret = avfilter_graph_parse_ptr(filter_graph, filter_descr.c_str(), &inputs, &outputs, NULL);
    if ( ret < 0 ) {
        return ret;
    }

    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);
    lastOutput = nullptr;
    lastInput = nullptr;
    abuffer = nullptr;
    vbuffer = nullptr;
    abuffersink = nullptr;
    vbuffersink = nullptr;
    return 0;
}

int FilterGraph::configure() {
    return avfilter_graph_config(filter_graph, NULL);
}

int FilterGraph::addFrame(const std::string& src_name, AVFrame* _Nullable frame, int flags) {
    AVFilterContext *buffer_ctx = instances[src_name];
    if ( buffer_ctx == nullptr ) {
        return AVERROR_FILTER_NOT_FOUND;
    }
    
    return av_buffersrc_add_frame_flags(buffer_ctx, frame, flags);
}

int FilterGraph::getFrame(const std::string& sink_name, AVFrame* _Nonnull frame) {
    AVFilterContext *buffersink_ctx = instances[sink_name];
    if ( buffersink_ctx == nullptr ) {
        return AVERROR_FILTER_NOT_FOUND;
    }
    
    return av_buffersink_get_frame(buffersink_ctx, frame);
}

void FilterGraph::release() {
    if ( outputs != nullptr ) {
        avfilter_inout_free(&outputs);
    }
    
    if ( inputs != nullptr ) {
        avfilter_inout_free(&inputs);
    }

    if ( filter_graph != nullptr ) {
        avfilter_graph_free(&filter_graph);
    }
}

}