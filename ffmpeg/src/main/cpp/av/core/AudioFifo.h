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
// Created on 2025/1/15.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef FFMPEGPROJ_AUDIOFIFO_H
#define FFMPEGPROJ_AUDIOFIFO_H

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavutil/audio_fifo.h"
#include "libavutil/samplefmt.h"
#include "libavutil/avutil.h"
#include "libavutil/rational.h"
}

namespace FFAV {

class AudioFifo {
public:
    AudioFifo();
    ~AudioFifo();

    int init(AVSampleFormat sample_fmt, int nb_channels, int nb_samples);

    int write(void** data, int nb_samples, int64_t pts);
    int read(void** data, int nb_samples, int64_t *pts_ptr);
    void clear();
    
    int getSize();  

private:
    AVAudioFifo* fifo = nullptr;        
    int64_t next_pts = AV_NOPTS_VALUE; // in time_base
    void release();
};

}
#endif //FFMPEGPROJ_AUDIOFIFO_H
