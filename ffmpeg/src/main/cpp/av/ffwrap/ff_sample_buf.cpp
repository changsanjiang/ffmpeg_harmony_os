//
// Created on 2025/7/3.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "ff_sample_buf.h"
#include <algorithm>
#include <libavutil/mem.h>   // av_malloc, av_free
#include <cstring>           // memset
#include <sys/stat.h>
#include "ff_throw.hpp"

namespace FFAV {

template<typename T>
static inline T clamp(T value, T min_val, T max_val) {
    return std::max(min_val, std::min(max_val, value));
}

static uint8_t** 
createBuf(int nb_frames, int nb_channels, int bytes_per_sample, bool interleaved) {
    if (interleaved) {
        // 交错格式: 每个frame的数据都交错在一个 buffer 中
        int buf_size = nb_frames * nb_channels * bytes_per_sample;
        uint8_t *data = (uint8_t *)av_mallocz(buf_size); // 需要清理为0;
        if (!data) return nullptr;

        // 返回双指针，方便兼容 av_samples_fill_arrays 的格式
        uint8_t **buf = (uint8_t **)av_malloc(sizeof(uint8_t *));
        if (!buf) {
            av_free(data);
            return nullptr;
        }

        buf[0] = data;
        return buf;
    } 
    else {
        // 非交错格式: 每个通道单独分配一块 buffer
        uint8_t **buf = (uint8_t **)av_mallocz(sizeof(uint8_t *) * nb_channels);
        if (!buf) return nullptr;

        for (int ch = 0; ch < nb_channels; ++ch) {
            buf[ch] = (uint8_t *)av_malloc(nb_frames * bytes_per_sample);
            if (!buf[ch]) {
                // 清理已分配的部分
                for (int i = 0; i < ch; ++i) {
                    av_free(buf[i]);
                }
                av_free(buf);
                return nullptr;
            }
        }

        return buf;
    }
}

void SampleBuf::resetBuf(uint8_t **buf, int nb_frames, int nb_channels, int bytes_per_sample, bool interleaved) {
    const int size = nb_frames * bytes_per_sample;
    if (interleaved) {
        memset(buf[0], 0, size * nb_channels);
    }
    else {
        for (int ch = 0; ch < nb_channels; ++ch) {
            memset(buf[ch], 0, size);
        }
    }
}

static void 
releaseBuf(uint8_t **buf, int nb_channels, bool interleaved) {
    if (!buf) return;

    if (interleaved) {
        av_free(buf[0]);
        av_free(buf);
    } 
    else {
        for (int ch = 0; ch < nb_channels; ++ch) {
            if (buf[ch]) {
                av_free(buf[ch]);
            }
        }
        av_free(buf);
    }
}

static void
mixBufInt16(int16_t **src, int16_t **dst, int nb_frames, int nb_channels, bool interleaved, float gain) {
    if (interleaved) {
        int total_samples = nb_frames * nb_channels;
        for (int i = 0; i < total_samples; ++i) {
            int scaled = static_cast<int>(src[0][i] * gain);
            int mixed = static_cast<int>(dst[0][i]) + scaled;
            dst[0][i] = clamp(mixed, -32768, 32767);
        }
    }
    else {
        for (int ch = 0; ch < nb_channels; ++ch) {
            for (int i = 0; i < nb_frames; ++i) {
                int scaled = static_cast<int>(src[ch][i] * gain);
                int mixed = static_cast<int>(dst[ch][i]) + scaled;
                dst[ch][i] = clamp(mixed, -32768, 32767);
            }
        }
    }
}

static void
mixBufFloat(float **src, float **dst, int nb_frames, int nb_channels, bool interleaved, float gain) {
    if (interleaved) {
        int total_samples = nb_frames * nb_channels;
        for (int i = 0; i < total_samples; ++i) {
            float scaled = src[0][i] * gain;
            float mixed = dst[0][i] + scaled;
            dst[0][i] = clamp(mixed, -1.0f, 1.0f);
        }
    } 
    else {
        for (int ch = 0; ch < nb_channels; ++ch) {
            for (int i = 0; i < nb_frames; ++i) {
                float scaled = src[ch][i] * gain;
                float mixed = dst[ch][i] + scaled;
                dst[ch][i] = clamp(mixed, -1.0f, 1.0f);
            }
        }
    }
}

SampleBuf::SampleBuf(int nb_frames, AVSampleFormat sample_format, int nb_channels):
    _nb_frames(nb_frames),
    _sample_format(sample_format), 
    _nb_channels(nb_channels),
    _bytes_per_sample(av_get_bytes_per_sample(sample_format)), 
    _interleaved(av_sample_fmt_is_planar(sample_format) == 0)  
{
    _buf = createBuf(nb_frames, nb_channels, _bytes_per_sample, _interleaved);
}

SampleBuf::~SampleBuf() {
    releaseBuf(_buf, _nb_channels, _interleaved);
}

void SampleBuf::reset() {
    const int size = _nb_frames * _bytes_per_sample;
    if (_interleaved) {
        memset(_buf[0], 0, size * _nb_channels);
    }
    else {
        for (int ch = 0; ch < _nb_channels; ++ch) {
            memset(_buf[ch], 0, size);
        }
    }
}

void SampleBuf::mixTo(uint8_t **dst, float gain) {
    if (!dst) return;

    if (_sample_format == AV_SAMPLE_FMT_S16 || _sample_format == AV_SAMPLE_FMT_S16P) {
        mixBufInt16((int16_t **)_buf, (int16_t **)dst, _nb_frames, _nb_channels, _interleaved, gain);
    } 
    else if (_sample_format == AV_SAMPLE_FMT_FLT || _sample_format == AV_SAMPLE_FMT_FLTP) {
        mixBufFloat((float **)_buf, (float **)dst, _nb_frames, _nb_channels, _interleaved,gain);
    }
    else {
        // 不支持的格式
        throw_error_fmt("mixBuf - Unsupported format: %d", _sample_format);
    }
}

} // namespace FFAV