//
// Created on 2025/1/15.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef FFMPEGPROJ_AUDIORENDERER_H
#define FFMPEGPROJ_AUDIORENDERER_H

#include <cstdint>
#include <functional>
#include <ohaudio/native_audiostream_base.h>

namespace CoreMedia {
    class AudioRenderer {
public:    
        AudioRenderer();
        ~AudioRenderer();
    
        OH_AudioStream_Result init(
            OH_AudioStream_SampleFormat sample_fmt, 
            int nb_channels, 
            int sample_rate,
            OH_AudioStream_Usage usage = AUDIOSTREAM_USAGE_MUSIC
        );
    
        using WriteDataCallback = std::function<OH_AudioData_Callback_Result(void* _Nullable user_data, void* _Nonnull data, int32_t size)>;
        void setWriteDataCallback(
            WriteDataCallback callback, 
            void* _Nullable user_data
        );
    
        OH_AudioStream_Result play();
        OH_AudioStream_Result pause();
        OH_AudioStream_Result stop();
        OH_AudioStream_Result flush();
    
        WriteDataCallback getWriteDataCallback();
        void* _Nullable getWriteDataCallbackUserData();
    
private:
        OH_AudioStreamBuilder* _Nullable builder = nullptr;
        OH_AudioRenderer* _Nullable audio_renderer = nullptr;
        WriteDataCallback write_data_cb = nullptr;
        void* write_data_cb_user_data = nullptr;
        void release();
    };
}

#endif //FFMPEGPROJ_AUDIORENDERER_H
