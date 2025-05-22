//
// Created on 2025/5/22.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef FFMPEG_HARMONY_OS_OHAUDIOSTREAMUTILS_H
#define FFMPEG_HARMONY_OS_OHAUDIOSTREAMUTILS_H

#include <ohaudio/native_audiostream_base.h>

extern "C" {
#include "libavutil/samplefmt.h"
}

namespace FFAV {

class OHAudioStreamUtils {
public:
    static AVSampleFormat ohToAVSampleFormat(OH_AudioStream_SampleFormat oh_fmt);
};

}

#endif //FFMPEG_HARMONY_OS_OHAUDIOSTREAMUTILS_H
