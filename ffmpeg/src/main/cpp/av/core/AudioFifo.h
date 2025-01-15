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
}

namespace CoreMedia {
    class AudioFifo {
public:
        AudioFifo();
        ~AudioFifo();
    
        int init(AVSampleFormat sample_fmt, int nb_channels, int nb_samples);
    
        int write(void** data, int nb_samples);
        int read(void** data, int nb_samples);
        
        int getSize();  
        AVSampleFormat getSampleFormat();
        int getChannels();
    
        void clear();
    
private:
        AVAudioFifo* fifo = nullptr;                    
        int nb_channels = 0;                            
        AVSampleFormat sample_fmt = AV_SAMPLE_FMT_NONE; 
        void release();
    };
}

#endif //FFMPEGPROJ_AUDIOFIFO_H
