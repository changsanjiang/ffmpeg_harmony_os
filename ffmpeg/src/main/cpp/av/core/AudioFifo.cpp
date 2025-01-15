//
// Created on 2025/1/15.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "AudioFifo.h"
#include <stdexcept>

namespace CoreMedia {
    AudioFifo::AudioFifo() = default;
    AudioFifo::~AudioFifo() { release(); }

    int AudioFifo::init(AVSampleFormat sample_fmt, int nb_channels, int nb_samples) {
        if ( fifo != nullptr ) {
            return AVERROR(EAGAIN);
        }
        
        fifo = av_audio_fifo_alloc(sample_fmt, nb_channels, nb_samples);
        if ( fifo == nullptr ) {
            return AVERROR(ENOMEM);
        }   
        this->sample_fmt = sample_fmt;
        this->nb_channels = nb_channels;
        return 0;
    }

    int AudioFifo::write(void** data, int nb_samples) {
        if ( fifo == nullptr ) {
            throw std::runtime_error("AVAudioFifo is not initialized");
        }
        return av_audio_fifo_write(fifo, data, nb_samples);
    }

    int AudioFifo::read(void** data, int nb_samples) {
        if ( fifo == nullptr ) {
            throw std::runtime_error("AVAudioFifo is not initialized");
        }
        return av_audio_fifo_read(fifo, data, nb_samples);
    }

    int AudioFifo::getSize() {
        return fifo != nullptr ? av_audio_fifo_size(fifo) : 0;
    }

    AVSampleFormat AudioFifo::getSampleFormat() {
        return sample_fmt;
    }

    int AudioFifo::getChannels() {
        return nb_channels;
    }

    void AudioFifo::clear() {
        if ( fifo != nullptr ) {
            av_audio_fifo_reset(fifo);
        }
    }

    void AudioFifo::release() {
        if ( fifo != nullptr ) {
            av_audio_fifo_free(fifo);
            fifo = nullptr;
        }
    }
}