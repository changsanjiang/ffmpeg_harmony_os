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
// Created on 2025/2/12.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef FFMPEG_HARMONY_OS_FFAUDIOPLAYER_H
#define FFMPEG_HARMONY_OS_FFAUDIOPLAYER_H

#include "av/audio/PlayWhenReadyChangeReason.h"
#include "napi/native_api.h"
#include <stdint.h>
#include <string>
#include "av/audio/AudioPlayer.h"

namespace FFAV {

class FFAudioPlayer {
public:
    static napi_value Init(napi_env env, napi_value exports);
    static void Destructor(napi_env env, void* nativeObject, void* finalize_hint);

private:
    static napi_value New(napi_env env, napi_callback_info info);
    static napi_value GetUrl(napi_env env, napi_callback_info info);
    static napi_value SetUrl(napi_env env, napi_callback_info info);
    static napi_value Prepare(napi_env env, napi_callback_info info);
    static napi_value Play(napi_env env, napi_callback_info info);
    static napi_value Pause(napi_env env, napi_callback_info info);
    static napi_value Stop(napi_env env, napi_callback_info info);
    static napi_value Seek(napi_env env, napi_callback_info info);
    
    static napi_value GetVolume(napi_env env, napi_callback_info info);
    static napi_value SetVolume(napi_env env, napi_callback_info info);
    static napi_value GetSpeed(napi_env env, napi_callback_info info);
    static napi_value SetSpeed(napi_env env, napi_callback_info info);
    static napi_value SetDefaultOutputDevice(napi_env env, napi_callback_info info);
    static napi_value GetPlayWhenReady(napi_env env, napi_callback_info info);
    static napi_value GetCurrentTime(napi_env env, napi_callback_info info);
    static napi_value GetDuration(napi_env env, napi_callback_info info);
    static napi_value GetPlayableDuration(napi_env env, napi_callback_info info);
    static napi_value GetError(napi_env env, napi_callback_info info);
    
    // events: playWhenReadyChange, durationChange, currentTimeChange, playableDurationChange, errorChange
    static napi_value On(napi_env env, napi_callback_info info);
    static napi_value Off(napi_env env, napi_callback_info info);
    
    FFAudioPlayer();
    ~FFAudioPlayer();
    
    std::string url;
    FFAV::AudioPlaybackOptions options { };
    FFAV::AudioPlayer* player = nullptr;
    std::atomic<int64_t> current_time_ms { 0 };
    std::atomic<int64_t> duration_ms { 0 };
    std::atomic<int64_t> playable_duration_ms { 0 };
    std::atomic<FFAV::Error*> cur_error { nullptr };
    std::atomic<bool> play_when_ready { false };
    
    float volume = 1;
    float speed = 1;
    OH_AudioDevice_Type device_type { OH_AudioDevice_Type::AUDIO_DEVICE_TYPE_DEFAULT };
    
    napi_threadsafe_function js_func_play_when_ready_callback = nullptr;
    napi_threadsafe_function js_func_duration_change_callback = nullptr;
    napi_threadsafe_function js_func_current_time_change_callback = nullptr;
    napi_threadsafe_function js_func_playable_duration_change_callback = nullptr;
    napi_threadsafe_function js_func_error_change_callback = nullptr;
    
    bool createPlayer();
    void prepare();
    void play();
    void pause();
    // 停止播放, 当前的播放资源及状态将会被清除和重置; error 也将被重置;
    void stop();
    void seek(int64_t time_ms);
    
    void setVolume(float volume);
    void setSpeed(float speed);
    void setDeviceType(int32_t device_type);

    void onPlayerEvent(std::shared_ptr<FFAV::EventMessage> msg);
    void onPlayWhenReadyChange(bool play_when_ready, PlayWhenReadyChangeReason reason);
    void onDurationChange(int64_t duration_ms);
    void onCurrentTimeChange(int64_t current_time_ms);
    void onPlayableDurationChange(int64_t playable_duration_ms);
    void onErrorChange(FFAV::Error* error); // nullable 
};

}
#endif //FFMPEG_HARMONY_OS_FFAUDIOPLAYER_H
