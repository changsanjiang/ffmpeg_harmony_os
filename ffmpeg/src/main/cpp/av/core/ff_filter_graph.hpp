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
// Created by sj on 2025/1/10.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef FFAV_FilterGraph_hpp
#define FFAV_FilterGraph_hpp

#include <string>
#include <unordered_map>
#include <functional>
#include "ff_types.hpp"

namespace FFAV {

class FilterGraph {
public:
    FilterGraph();
    ~FilterGraph();

    int init();

    int addAudioBufferSourceFilter(const std::string& name, const AVRational time_base, int sample_rate, AVSampleFormat sample_fmt, const std::string& ch_layout_desc);
    int addVideoBufferSourceFilter(const std::string& name, const AVRational time_base, int width, int height, AVPixelFormat pix_fmt, const AVRational sar, const AVRational frame_rate);
    int addBufferSourceFilter(const std::string& name, AVMediaType type, const AVBufferSrcParameters* _Nonnull params);

    int addAudioBufferSinkFilter(const std::string& name, const int* _Nullable sample_rates, const AVSampleFormat* _Nullable sample_fmts, const std::string& channel_layout);
    
    int addAudioBufferSinkFilter(const std::string& name, const int sample_rate, const AVSampleFormat sample_fmt, const std::string& channel_layout);
    
    int addVideoBufferSinkFilter(const std::string& name, const AVPixelFormat* _Nullable pix_fmts);

    
    using ParseLinker = std::function<int(FilterGraph*_Nonnull filter_graph, AVFilterInOut*_Nullable inputs, AVFilterInOut*_Nullable outputs)>;

    int parse(const std::string& filter_descr, ParseLinker linker = nullptr);
    
    int createAudioBufferSourceFilter(const std::string& name, const AVRational time_base, int sample_rate, AVSampleFormat sample_fmt, const std::string& ch_layout_desc, AVFilterContext *_Nullable*_Nullable filter_ctx);
    int createVideoBufferSourceFilter(const std::string& name, const AVRational time_base, int width, int height, AVPixelFormat pix_fmt, const AVRational sar, const AVRational frame_rate, AVFilterContext *_Nullable*_Nullable filter_ctx);
    int createBufferSourceFilter(const std::string& name, AVMediaType type, const AVBufferSrcParameters* _Nonnull params, AVFilterContext *_Nullable*_Nullable filter_ctx);

    int createAudioBufferSinkFilter(const std::string& name, const int sample_rate, const AVSampleFormat sample_fmt, const std::string& ch_layout_desc, AVFilterContext *_Nullable*_Nullable filter_ctx);
    
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

    /**
     * Send a command to one or more filter instances.
     *
     * @param graph  the filter graph
     * @param target the filter(s) to which the command should be sent
     *               "all" sends to all filters
     *               otherwise it can be a filter or filter instance name
     *               which will send the command to all matching filters.
     * @param cmd    the command to send, for handling simplicity all commands must be alphanumeric only
     * @param arg    the argument for the command
     * @param res    a buffer with size res_size where the filter(s) can return a response.
     *
     * @returns >=0 on success otherwise an error code.
     *              AVERROR(ENOSYS) on unsupported commands
     */
    int sendCommand(const std::string& target_name, const std::string& cmd, const std::string& arg, int flags = AVFILTER_CMD_FLAG_ONE);
    
private:
    int addBufferSourceFilter(const std::string& name, AVFilterContext* _Nonnull buffer_ctx);
    int addBufferSinkFilter(const std::string& name, AVFilterContext* _Nonnull buffersink_ctx);
    void release();
    
private:
    AVFilterGraph* _Nullable _filter_graph = nullptr;
    AVFilterInOut* _Nullable _outputs = nullptr;
    AVFilterInOut* _Nullable _inputs = nullptr;
    AVFilterInOut* _Nullable _lastOutput = nullptr;
    AVFilterInOut* _Nullable _lastInput = nullptr;
    
    const AVFilter* _Nullable _abuffer = nullptr;
    const AVFilter* _Nullable _vbuffer = nullptr;

    const AVFilter* _Nullable _abuffersink = nullptr;
    const AVFilter* _Nullable _vbuffersink = nullptr;
};

}
#endif //FFAV_FilterGraph_hpp
