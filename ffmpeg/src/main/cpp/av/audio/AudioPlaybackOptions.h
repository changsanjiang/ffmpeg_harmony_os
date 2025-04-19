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
// Created on 2025/3/5.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef FFMPEG_HARMONY_OS_AUDIOPLAYBACKOPTIONS_H
#define FFMPEG_HARMONY_OS_AUDIOPLAYBACKOPTIONS_H

#include <map>
#include <ohaudio/native_audiostream_base.h>
#include <stdint.h>

namespace FFAV {

struct AudioPlaybackOptions {
    int64_t start_time_position_ms;
    std::map<std::string, std::string> http_options;
    OH_AudioStream_Usage stream_usage;
    
    void reset() {
        start_time_position_ms = 0;
        http_options.clear();
        stream_usage = AUDIOSTREAM_USAGE_MUSIC;
    }
};

}

#endif //FFMPEG_HARMONY_OS_AUDIOPLAYBACKOPTIONS_H
