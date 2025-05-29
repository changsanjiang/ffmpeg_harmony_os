//
// Created on 2025/5/22.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "OHUtils.hpp"
#include <stdexcept>

namespace FFAV {

AVSampleFormat Conversion::toAVFormat(OH_AudioStream_SampleFormat oh_fmt) {
    switch (oh_fmt) {
        case OH_AudioStream_SampleFormat::AUDIOSTREAM_SAMPLE_U8:
            return AV_SAMPLE_FMT_U8;
        case OH_AudioStream_SampleFormat::AUDIOSTREAM_SAMPLE_S16LE:
            return AV_SAMPLE_FMT_S16;
        case OH_AudioStream_SampleFormat::AUDIOSTREAM_SAMPLE_S32LE:
            return AV_SAMPLE_FMT_S32;
        case OH_AudioStream_SampleFormat::AUDIOSTREAM_SAMPLE_S24LE: // Unsupported
            return AV_SAMPLE_FMT_NONE;
        default:
            return AV_SAMPLE_FMT_NONE; 
    }
}

OH_AudioStream_SampleFormat Conversion::toOHFormat(AVSampleFormat ff_fmt) {
    switch (ff_fmt) {
        case AV_SAMPLE_FMT_U8:
            return OH_AudioStream_SampleFormat::AUDIOSTREAM_SAMPLE_U8;
        case AV_SAMPLE_FMT_S16:
            return OH_AudioStream_SampleFormat::AUDIOSTREAM_SAMPLE_S16LE;
        case AV_SAMPLE_FMT_S32:
            return OH_AudioStream_SampleFormat::AUDIOSTREAM_SAMPLE_S32LE;
        default:
            throw std::runtime_error("Unsupported AVSampleFormat for OH conversion");
    }
}

} // namespace FFAV