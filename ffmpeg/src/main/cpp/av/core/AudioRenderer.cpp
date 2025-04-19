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

#include "AudioRenderer.h"
#include <cstdint>
#include <ohaudio/native_audiorenderer.h>
#include <ohaudio/native_audiostreambuilder.h>
#include <stdexcept>

namespace FFAV {

AudioRenderer::AudioRenderer() = default;
AudioRenderer::~AudioRenderer() { release(); }

static int32_t onStreamEvent(
    OH_AudioRenderer* renderer,
    void* userData,
    OH_AudioStream_Event event
);

static int32_t onInterruptEvent(
    OH_AudioRenderer* renderer,
    void* userData,
    OH_AudioInterrupt_ForceType type,
    OH_AudioInterrupt_Hint hint
);

static int32_t onError(
    OH_AudioRenderer* renderer,
    void* userData,
    OH_AudioStream_Result error
);

static OH_AudioData_Callback_Result onWriteData( 
    OH_AudioRenderer* renderer, 
    void* userData,
    void* audioData, 
    int32_t audioDataSize
);

static void onOutputDeviceChange(
    OH_AudioRenderer* renderer,
    void* userData,
    OH_AudioStream_DeviceChangeReason reason
);

OH_AudioStream_Result AudioRenderer::init(
    OH_AudioStream_SampleFormat sample_fmt, 
    int sample_rate,
    int nb_channels, 
    OH_AudioStream_Usage usage
) {
    if ( builder != nullptr ) {
        throw std::runtime_error("AudioFifo is already initialized");
    }

    OH_AudioStream_Result res;
    OH_AudioStreamBuilder* builder = nullptr;
    OH_AudioRenderer_Callbacks renderer_callbacks;
    OH_AudioRenderer* audio_renderer = nullptr;
    int frame_size = 0;

    res = OH_AudioStreamBuilder_Create(&builder, AUDIOSTREAM_TYPE_RENDERER);
    if ( res != AUDIOSTREAM_SUCCESS ) {
        goto end;
    }
    
    // 设置音频采样率
    res = OH_AudioStreamBuilder_SetSamplingRate(builder, sample_rate);
    if ( res != AUDIOSTREAM_SUCCESS ) {
        goto end;
    }

    // 设置音频声道
    res = OH_AudioStreamBuilder_SetChannelCount(builder, nb_channels);
    if ( res != AUDIOSTREAM_SUCCESS ) {
        goto end;
    }

    // 设置音频采样格式
    res = OH_AudioStreamBuilder_SetSampleFormat(builder, sample_fmt);
    if ( res != AUDIOSTREAM_SUCCESS ) {
        goto end;
    }

    // 设置音频流的编码类型
    res = OH_AudioStreamBuilder_SetEncodingType(builder, AUDIOSTREAM_ENCODING_TYPE_RAW);
    if ( res != AUDIOSTREAM_SUCCESS ) {
        goto end;
    }

    // 设置输出音频流的工作场景
    res = OH_AudioStreamBuilder_SetRendererInfo(builder, usage);
    if ( res != AUDIOSTREAM_SUCCESS ) {
        goto end;
    }

    renderer_callbacks.OH_AudioRenderer_OnStreamEvent = onStreamEvent;
    renderer_callbacks.OH_AudioRenderer_OnInterruptEvent = onInterruptEvent;
    renderer_callbacks.OH_AudioRenderer_OnError = onError;
    renderer_callbacks.OH_AudioRenderer_OnWriteData = nullptr;
    res = OH_AudioStreamBuilder_SetRendererCallback(builder, renderer_callbacks, this);  
    if ( res != AUDIOSTREAM_SUCCESS ) {
        goto end;
    }
    
    res = OH_AudioStreamBuilder_SetRendererWriteDataCallback(builder, onWriteData, this);
    if ( res != AUDIOSTREAM_SUCCESS ) {
        goto end;
    }
    
    res = OH_AudioStreamBuilder_SetRendererOutputDeviceChangeCallback(builder, onOutputDeviceChange, this);
    if ( res != AUDIOSTREAM_SUCCESS ) {
        goto end;
    }

    res = OH_AudioStreamBuilder_GenerateRenderer(builder, &audio_renderer);
    if ( res != AUDIOSTREAM_SUCCESS ) {
        goto end;
    }

    res = OH_AudioRenderer_GetFrameSizeInCallback(audio_renderer, &frame_size);
    if ( res != AUDIOSTREAM_SUCCESS ) {
        goto end;
    }
    
end:
    if ( res != AUDIOSTREAM_SUCCESS ) {
        if ( audio_renderer != nullptr ) {
            OH_AudioRenderer_Release(audio_renderer);
        }
    
        if ( builder != nullptr ) {
            OH_AudioStreamBuilder_Destroy(builder);
        }   
        return res;
    }

    this->audio_renderer = audio_renderer;     
    this->builder = builder;
    this->sample_fmt = sample_fmt;
    this->nb_channels = nb_channels;
    this->sample_rate = sample_rate;
    this->frame_size  = frame_size;
    return AUDIOSTREAM_SUCCESS;
}

OH_AudioStream_Result AudioRenderer::play() {
    if ( audio_renderer == nullptr ) {
        throw std::runtime_error("AudioRenderer is not initialized");
    }
    return OH_AudioRenderer_Start(audio_renderer);
}

OH_AudioStream_Result AudioRenderer::pause() {
    if ( audio_renderer == nullptr ) {
        throw std::runtime_error("AudioRenderer is not initialized");
    }
    return OH_AudioRenderer_Pause(audio_renderer);
}

OH_AudioStream_Result AudioRenderer::stop() {
    if ( audio_renderer == nullptr ) {
        throw std::runtime_error("AudioRenderer is not initialized");
    }
    return OH_AudioRenderer_Stop(audio_renderer);
}

OH_AudioStream_Result AudioRenderer::flush() {
    if ( audio_renderer == nullptr ) {
        throw std::runtime_error("AudioRenderer is not initialized");
    }
    return OH_AudioRenderer_Flush(audio_renderer);
}

OH_AudioStream_Result AudioRenderer::getCurrentState(OH_AudioStream_State* state) {
    if ( audio_renderer == nullptr ) {
        throw std::runtime_error("AudioRenderer is not initialized");
    }
    return OH_AudioRenderer_GetCurrentState(audio_renderer, state);
}

// [0.0, 1.0]
OH_AudioStream_Result AudioRenderer::setVolume(float volume) {
    if ( audio_renderer == nullptr ) {
        throw std::runtime_error("AudioRenderer is not initialized");
    }
    return OH_AudioRenderer_SetVolume(audio_renderer, volume);
}

OH_AudioStream_Result AudioRenderer::getVolume(float* volume_prt) {
    if ( audio_renderer == nullptr ) {
        throw std::runtime_error("AudioRenderer is not initialized");
    }
    return OH_AudioRenderer_GetVolume(audio_renderer, volume_prt);
}

// [0.25, 4.0]
OH_AudioStream_Result AudioRenderer::setSpeed(float speed) {
    if ( audio_renderer == nullptr ) {
        throw std::runtime_error("AudioRenderer is not initialized");
    }
    return OH_AudioRenderer_SetSpeed(audio_renderer, speed);
}

OH_AudioStream_Result AudioRenderer::getSpeed(float* speed_ptr) {
    if ( audio_renderer == nullptr ) {
        throw std::runtime_error("AudioRenderer is not initialized");
    }
    return OH_AudioRenderer_GetSpeed(audio_renderer, speed_ptr);
}

OH_AudioStream_Result AudioRenderer::setDefaultOutputDevice(OH_AudioDevice_Type device_type) {
    if ( audio_renderer == nullptr ) {
        throw std::runtime_error("AudioRenderer is not initialized");
    }
    return OH_AudioRenderer_SetDefaultOutputDevice(audio_renderer, device_type);
}

void AudioRenderer::setWriteDataCallback(AudioRenderer::WriteDataCallback callback) {
    write_data_cb = callback;
}

AudioRenderer::WriteDataCallback AudioRenderer::getWriteDataCallback() {
    return write_data_cb;
}

void AudioRenderer::setInterruptEventCallback(AudioRenderer::InterruptEventCallback callback) {
    interrupt_event_callback = callback;
}

AudioRenderer::InterruptEventCallback AudioRenderer::getInterruptEventCallback() {
    return interrupt_event_callback;
}

void AudioRenderer::setErrorCallback(AudioRenderer::ErrorCallback callback) {
    error_callback = callback;
}

AudioRenderer::ErrorCallback AudioRenderer::getErrorCallback() {
    return error_callback;
}

void AudioRenderer::setOutputDeviceChangeCallback(AudioRenderer::OutputDeviceChangeCallback callback) {
    output_device_change_callback = callback;
}

AudioRenderer::OutputDeviceChangeCallback AudioRenderer::getOutputDeviceChangeCallback() {
    return output_device_change_callback;
}

int AudioRenderer::getFrameSize() {
    return frame_size;
} 

void AudioRenderer::release() {
    if ( audio_renderer != nullptr ) {
        OH_AudioRenderer_Release(audio_renderer);
        audio_renderer = nullptr;
    }

    if ( builder != nullptr ) {
        OH_AudioStreamBuilder_Destroy(builder);
        builder = nullptr;
    }
}

// 自定义音频流事件函数
static int32_t onStreamEvent(
    OH_AudioRenderer* renderer,
    void* userData,
    OH_AudioStream_Event event
) {
    // 根据event表示的音频流事件信息，更新播放器状态和界面
    return 0;
}

// 自定义音频中断事件函数
static int32_t onInterruptEvent(
    OH_AudioRenderer* renderer,
    void* userData,
    OH_AudioInterrupt_ForceType type,
    OH_AudioInterrupt_Hint hint
) {
    // 根据type和hint表示的音频中断信息，更新播放器状态和界面
    AudioRenderer* player = static_cast<AudioRenderer*>(userData);
    if ( player != nullptr ) {
        AudioRenderer::InterruptEventCallback callback = player->getInterruptEventCallback();
        if ( callback ) {
            callback(type, hint);
        }
    }
    return 0;
}

// 自定义异常回调函数
static int32_t onError(
    OH_AudioRenderer* renderer,
    void* userData,
    OH_AudioStream_Result error
) {
    // 根据error表示的音频异常信息，做出相应的处理
    AudioRenderer* player = static_cast<AudioRenderer*>(userData);
    if ( player != nullptr ) {
        AudioRenderer::ErrorCallback callback = player->getErrorCallback();
        if ( callback ) {
            callback(error);
        }
    }
    return 0;
}

// 自定义写入数据函数
static OH_AudioData_Callback_Result onWriteData( 
    OH_AudioRenderer* renderer, 
    void* userData,
    void* audioData, 
    int32_t audioDataSize
) {
    AudioRenderer* player = static_cast<AudioRenderer*>(userData);
    if ( player != nullptr ) {
        AudioRenderer::WriteDataCallback callback = player->getWriteDataCallback();
        if ( callback ) {
            return callback(audioData, audioDataSize);
        }
    }   
    // 将待播放的数据，按audioDataSize长度写入audioData
    // 如果开发者不希望播放某段audioData，返回AUDIO_DATA_CALLBACK_RESULT_INVALID即可
    return AUDIO_DATA_CALLBACK_RESULT_INVALID;
}

static void onOutputDeviceChange(
    OH_AudioRenderer* renderer,
    void* userData,
    OH_AudioStream_DeviceChangeReason reason
) {
    AudioRenderer* player = static_cast<AudioRenderer*>(userData);
    if ( player != nullptr ) {
        auto callback = player->getOutputDeviceChangeCallback();
        if ( callback ) {
            return callback(reason);
        }
    }   
}

}