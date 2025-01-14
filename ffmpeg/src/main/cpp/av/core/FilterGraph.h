//
// Created on 2025/1/10.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef FFMPEGPROJ_FILTERGRAPH_H
#define FFMPEGPROJ_FILTERGRAPH_H

#include <string>
#include <unordered_map>

extern "C" {
#include "libavfilter/avfilter.h"
#include "libavutil/avutil.h"
#include "libavutil/samplefmt.h"
#include "libavutil/pixfmt.h"
#include "libavfilter/buffersrc.h"
#include "libavutil/frame.h"
#include "libavutil/rational.h"
}

namespace CoreMedia {
    class FilterGraph {
public:
        FilterGraph();
        ~FilterGraph();
    
        int init();
    
        int addAudioBufferSourceFilter(const std::string& name, AVRational time_base, int sample_rate, AVSampleFormat sample_fmt, const AVChannelLayout ch_layout);
        int addVideoBufferSourceFilter(const std::string& name, const AVRational time_base, int width, int height, AVPixelFormat pix_fmt, const AVRational sar, const AVRational frame_rate);
        int addBufferSourceFilter(const std::string& name, AVMediaType type, const AVBufferSrcParameters* params);
    
        int addAudioBufferSinkFilter(const std::string& name, const int* _Nullable sample_rates, const AVSampleFormat* _Nullable sample_fmts, const std::string& channel_layout);
        int addVideoBufferSinkFilter(const std::string& name, const AVPixelFormat* _Nullable pix_fmts);
    
        int parse(const std::string& filter_descr);
        int configure();
        
        /**
         * av_buffersrc_add_frame_flags
         * 
         * Add a frame to the buffer source.
         *
         * By default, if the frame is reference-counted, this function will take
         * ownership of the reference(s) and reset the frame. This can be controlled
         * using the flags.
         *
         * If this function returns an error, the input frame is not touched.
         *
         * @param buffer_src  pointer to a buffer source context
         * @param frame       a frame, or NULL to mark EOF
         * @param flags       a combination of AV_BUFFERSRC_FLAG_*
         * @return            >= 0 in case of success, a negative AVERROR code
         *                    in case of failure
         */
        int addFrame(const std::string& src_name, AVFrame* _Nullable frame, int flags = AV_BUFFERSRC_FLAG_KEEP_REF);
    
        /**
         * av_buffersink_get_frame
         * 
         * Get a frame with filtered data from sink and put it in frame.
         *
         * @param ctx pointer to a context of a buffersink or abuffersink AVFilter.
         * @param frame pointer to an allocated frame that will be filled with data.
         *              The data must be freed using av_frame_unref() / av_frame_free()
         *
         * @return
         *         - >= 0 if a frame was successfully returned.
         *         - AVERROR(EAGAIN) if no frames are available at this point; more
         *           input frames must be added to the filtergraph to get more output.
         *         - AVERROR_EOF if there will be no more output frames on this sink.
         *         - A different negative AVERROR code in other failure cases.
         */
        int getFrame(const std::string& sink_name, AVFrame* _Nonnull frame);

private:
        AVFilterGraph* _Nullable filter_graph = nullptr;
        AVFilterInOut* _Nullable outputs = nullptr;
        AVFilterInOut* _Nullable inputs = nullptr;
        AVFilterInOut* _Nullable lastOutput = nullptr;
        AVFilterInOut* _Nullable lastInput = nullptr;
        std::unordered_map<std::string, AVFilterContext*> instances;
        
        const AVFilter* _Nullable abuffer = nullptr;
        const AVFilter* _Nullable vbuffer = nullptr;

        const AVFilter* _Nullable abuffersink = nullptr;
        const AVFilter* _Nullable vbuffersink = nullptr;
        int addBufferSourceFilter(const std::string& name, AVFilterContext* buffer_ctx);
        int addBufferSinkFilter(const std::string& name, AVFilterContext* buffersink_ctx);
        void release();
    };
}

#endif //FFMPEGPROJ_FILTERGRAPH_H
