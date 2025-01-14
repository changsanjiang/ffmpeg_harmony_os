//
// Created on 2025/1/8.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "AudioResampler.h"

extern "C" {
    #include <libswresample/swresample.h>
    #include <libavutil/avutil.h>
    #include <libavutil/frame.h>
    #include <libavutil/samplefmt.h>
    #include <libavutil/channel_layout.h>
    #include <libavutil/mem.h>
}

namespace CoreMedia {
    AudioResampler::AudioResampler(): swr_ctx(nullptr) { }
    
    AudioResampler::~AudioResampler() { release(); }

    int AudioResampler::init(const AVChannelLayout& in_ch_layout, int in_sample_rate, AVSampleFormat in_sample_fmt,
                                const AVChannelLayout& out_ch_layout, int out_sample_rate, AVSampleFormat out_sample_fmt) {
        if ( swr_ctx != nullptr ) {
            return AVERROR(EAGAIN);
        }
    
        constexpr int min_sample_rate = 1;
        if ( in_sample_rate < min_sample_rate || out_sample_rate < min_sample_rate ) {
            return AVERROR(EINVAL);
        }
    
        if ( in_ch_layout.nb_channels <= 0 || out_ch_layout.nb_channels <= 0 ) {
            return AVERROR(EINVAL);
        }

        /* create resampler context */
        swr_ctx = swr_alloc();
        if ( swr_ctx == nullptr ) {
            return AVERROR(ENOMEM);
        }

        /* set options */
        int error = swr_alloc_set_opts2(&swr_ctx,
            &out_ch_layout, out_sample_fmt, out_sample_rate,
            &in_ch_layout, in_sample_fmt, in_sample_rate,
            0, nullptr);
        if ( error < 0 ) {
            return error;
        }

        /* initialize the resampling context */
        error = swr_init(swr_ctx);
        if (error < 0) {
            return error;
        }
        
        error = av_channel_layout_copy(&this->out_ch_layout, &out_ch_layout);
        if (error < 0) {
            return error;
        }
        this->out_sample_rate = out_sample_rate;
        this->out_sample_fmt = out_sample_fmt;
        return 0;
    }

    // 执行重采样
    int AudioResampler::resample(AVFrame* _Nonnull out_frame, const AVFrame* _Nonnull in_frame) {
        if ( swr_ctx == nullptr ) {
            return AVERROR(EINVAL);
        }

        // swr_convert_frame
        // Input and output AVFrames must have channel_layout, sample_rate and format set.
        out_frame->ch_layout = out_ch_layout;
        out_frame->sample_rate = out_sample_rate;
        out_frame->format = out_sample_fmt;

        if ( in_frame->nb_samples == 0 ) {
            out_frame->nb_samples = 0;
            return 0;
        }
    
        // Allocate buffer for output frame
        int error = av_frame_get_buffer(out_frame, 0);
        if ( error < 0 ) {
            return error;
        }
    
        // Perform resampling
        error = swr_convert_frame(swr_ctx, out_frame, in_frame);
        return error;
    }

    // 释放资源
    void AudioResampler::release() {
        if (swr_ctx) {
            swr_free(&swr_ctx);
        }

        av_channel_layout_uninit(&out_ch_layout);
    }
}