//
// Created on 2025/5/22.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "OHAudioStreamUtils.hpp"
#include <stdexcept>

namespace FFAV {

AVSampleFormat OHAudioStreamUtils::ohToAVSampleFormat(OH_AudioStream_SampleFormat oh_fmt) {
    switch (oh_fmt) {
        case AUDIOSTREAM_SAMPLE_U8:
            return AV_SAMPLE_FMT_U8;  // 无符号 8-bit
        case AUDIOSTREAM_SAMPLE_S16LE:
            return AV_SAMPLE_FMT_S16; // 有符号 16-bit
        case AUDIOSTREAM_SAMPLE_S32LE:
            return AV_SAMPLE_FMT_S32; // 有符号 32-bit
        case AUDIOSTREAM_SAMPLE_S24LE:
            throw std::runtime_error("Unsupported format: AUDIOSTREAM_SAMPLE_S24LE (24-bit PCM has no direct AVSampleFormat)");
        default:
            throw std::runtime_error("Unknown OH_AudioStream_SampleFormat value");
    }
}

}