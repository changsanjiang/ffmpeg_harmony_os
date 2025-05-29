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

namespace Conversion {
    AVSampleFormat toAVFormat(OH_AudioStream_SampleFormat oh_fmt);
    OH_AudioStream_SampleFormat toOHFormat(AVSampleFormat ff_fmt);
} // namespace Conversion

} // namespace FFAV

#endif //FFMPEG_HARMONY_OS_OHAUDIOSTREAMUTILS_H
