//
// Created on 2025/1/15.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "AudioRenderer.h"
#include <ohaudio/native_audiorenderer.h>
#include <ohaudio/native_audiostreambuilder.h>
#include <stdexcept>

namespace CoreMedia {
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

    OH_AudioStream_Result AudioRenderer::init(
        OH_AudioStream_SampleFormat sample_fmt, 
        int nb_channels, 
        int sample_rate,
        OH_AudioStream_Usage usage
    ) {
        if ( builder != nullptr ) {
            throw std::runtime_error("AudioFifo is already initialized");
        }
    
        OH_AudioStreamBuilder* builder = nullptr;
        OH_AudioRenderer_Callbacks renderer_callbacks;
        OH_AudioRenderer* audio_renderer = nullptr;
        OH_AudioStream_Result res;
    
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
    
        res = OH_AudioStreamBuilder_GenerateRenderer(builder, &audio_renderer);

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
        return AUDIOSTREAM_SUCCESS;
    }

    void AudioRenderer::setWriteDataCallback(
        WriteDataCallback callback, 
        void* _Nullable user_data
    ) {
        write_data_cb = callback;
        write_data_cb_user_data = user_data;
    }

    AudioRenderer::WriteDataCallback AudioRenderer::getWriteDataCallback() {
        return write_data_cb;
    }

    void* _Nullable AudioRenderer::getWriteDataCallbackUserData() {
        return write_data_cb_user_data;
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
        return 0;
    }

    // 自定义异常回调函数
    static int32_t onError(
        OH_AudioRenderer* renderer,
        void* userData,
        OH_AudioStream_Result error
    ) {
        // 根据error表示的音频异常信息，做出相应的处理
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
            void* user_data = player->getWriteDataCallbackUserData();
            return callback(user_data, audioData, audioDataSize);
        }   
        // 将待播放的数据，按audioDataSize长度写入audioData
        // 如果开发者不希望播放某段audioData，返回AUDIO_DATA_CALLBACK_RESULT_INVALID即可
        return AUDIO_DATA_CALLBACK_RESULT_INVALID;
    }
}