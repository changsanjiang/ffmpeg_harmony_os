//
// Created on 2025/7/3.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef FFMPEG_HARMONY_OS_LAME_FF_SAMPLE_BUF_H
#define FFMPEG_HARMONY_OS_LAME_FF_SAMPLE_BUF_H

#include <stdint.h>
#include "ff_types.hpp"
namespace FFAV {

class SampleBuf {
public:
    SampleBuf(int nb_frames, AVSampleFormat sample_format, int nb_channels);
    ~SampleBuf();
    
    SampleBuf(const SampleBuf&) = delete;
    SampleBuf& operator=(const SampleBuf&) = delete;

    void reset();
    void mixTo(uint8_t **dst, float gain);
    
    uint8_t** data() const { return _buf; }    
    int nbFrames() const { return _nb_frames; }

    static void resetBuf(uint8_t **buf, int nb_frames, int nb_channels, int bytes_per_sample, bool interleaved);
    
private:
    int _nb_frames;
    AVSampleFormat _sample_format;
    int _nb_channels;
    int _bytes_per_sample;
    bool _interleaved;
    uint8_t **_buf;
};

} // namespace FFAV

#endif //FFMPEG_HARMONY_OS_LAME_FF_SAMPLE_BUF_H
