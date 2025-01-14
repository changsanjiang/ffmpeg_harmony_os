//
// Created on 2025/1/8.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef FFMPEGPROJ_AUDIORESAMPLER_H
#define FFMPEGPROJ_AUDIORESAMPLER_H

extern "C" {
    #include <libswresample/swresample.h>
    #include <libavutil/frame.h>
    #include <libavutil/samplefmt.h>
}

namespace CoreMedia {
    class AudioResampler {
public:
        AudioResampler();
        ~AudioResampler();
    
        // 初始化
        int init(const AVChannelLayout& in_ch_layout, int in_sample_rate, AVSampleFormat in_sample_fmt,
                 const AVChannelLayout& out_ch_layout, int out_sample_rate, AVSampleFormat out_sample_fmt);
    
        // 重采样
        int resample(AVFrame* _Nonnull out_frame, const AVFrame* _Nonnull in_frame);
    
private:
        SwrContext* _Nullable swr_ctx;
        AVChannelLayout out_ch_layout;
        int out_sample_rate;
        AVSampleFormat out_sample_fmt;
        void release();
    };
}

#endif //FFMPEGPROJ_AUDIORESAMPLER_H
