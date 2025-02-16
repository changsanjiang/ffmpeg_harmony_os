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
// Created on 2025/1/17.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef FFMPEGPROJ_AUDIOUTILS_H
#define FFMPEGPROJ_AUDIOUTILS_H

#include "av/core/AudioFifo.h"
#include "av/core/MediaDecoder.h"
#include "av/core/FilterGraph.h"

namespace FFAV {

class AudioUtils {
public:
    static int transcode(
        AVPacket* _Nullable pkt,
        MediaDecoder* _Nonnull decoder, 
        AVFrame* _Nonnull dec_frame,
        FilterGraph* _Nonnull filter_graph,
        AVFrame* _Nonnull filt_frame,
        const std::string& buf_src_name,
        const std::string& buf_sink_name,
        AudioFifo* _Nonnull fifo
    );

private:
    static int process_decoded_frames(
        MediaDecoder* _Nonnull decoder, 
        AVFrame* _Nonnull dec_frame,
        FilterGraph* _Nonnull filter_graph,
        AVFrame* _Nonnull filt_frame,
        const std::string& buf_src_name,
        const std::string& buf_sink_name,
        AudioFifo* _Nonnull fifo
    );
    
    static int process_filter_frame(
        AVFrame* _Nullable frame,
        FilterGraph* _Nonnull filter_graph,
        AVFrame* _Nonnull filt_frame,
        const std::string& buf_src_name,
        const std::string& buf_sink_name,
        AudioFifo* _Nonnull fifo
    );
    
    static int transfer_filtered_frames(
        FilterGraph* _Nonnull filter_graph,
        AVFrame* _Nonnull filt_frame,
        const std::string& buf_sink_name,
        AudioFifo* _Nonnull fifo
    );
};

}

#endif //FFMPEGPROJ_AUDIOUTILS_H

