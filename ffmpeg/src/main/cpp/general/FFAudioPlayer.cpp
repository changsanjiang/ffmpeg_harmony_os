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

#include "FFAudioPlayer.h"
#include "av/audio/EventMessage.h"
#include <stdint.h>

//#define DEBUG

#include "av/audio/PlayWhenReadyChangeReason.h"

namespace FFAV {

//struct FFAudioPlayer::FFAudioPlaybackOptions { 
//    int64_t start_time_position_ms;
//};

napi_value FFAudioPlayer::Init(napi_env env, napi_value exports) {
#ifdef DEBUG
    ff_console_print("AAAA: FFAudioPlayer::Init");
#endif
    
    napi_property_descriptor properties[] = {
        { "url", nullptr, nullptr, GetUrl, SetUrl, nullptr, napi_default, nullptr},
        { "setUrl", nullptr, SetUrl, nullptr, nullptr, nullptr, napi_default, nullptr},
        { "volume", nullptr, nullptr, GetVolume, SetVolume, nullptr, napi_default, nullptr},
        { "speed", nullptr, nullptr, GetSpeed, SetSpeed, nullptr, napi_default, nullptr},
        { "playWhenReady", nullptr, nullptr, GetPlayWhenReady, nullptr, nullptr, napi_default, nullptr},
        { "duration", nullptr, nullptr, GetDuration, nullptr, nullptr, napi_default, nullptr},
        { "currentTime", nullptr, nullptr, GetCurrentTime, nullptr, nullptr, napi_default, nullptr},
        { "playableDuration", nullptr, nullptr, GetPlayableDuration, nullptr, nullptr, napi_default, nullptr},
        { "error", nullptr, nullptr, GetError, nullptr, nullptr, napi_default, nullptr},
        { "setDefaultOutputDevice", nullptr, SetDefaultOutputDevice, nullptr, nullptr, nullptr, napi_default, nullptr},
        { "prepare", nullptr, Prepare, nullptr, nullptr, nullptr, napi_default, nullptr},
        { "play", nullptr, Play, nullptr, nullptr, nullptr, napi_default, nullptr},
        { "pause", nullptr, Pause, nullptr, nullptr, nullptr, napi_default, nullptr},
        { "stop", nullptr, Stop, nullptr, nullptr, nullptr, napi_default, nullptr},
        { "seek", nullptr, Seek, nullptr, nullptr, nullptr, napi_default, nullptr},
        { "on", nullptr, On, nullptr, nullptr, nullptr, napi_default, nullptr},
        { "off", nullptr, Off, nullptr, nullptr, nullptr, napi_default, nullptr},
    };

    size_t property_count = sizeof(properties) / sizeof(properties[0]);
    napi_value cons;
    napi_define_class(env, "FFAudioPlayer", NAPI_AUTO_LENGTH, New, nullptr, property_count, properties, &cons);
    napi_set_named_property(env, exports, "FFAudioPlayer", cons);
    return exports;
}

void FFAudioPlayer::Destructor(napi_env env, void* nativeObject, [[maybe_unused]] void* finalize_hint) {
#ifdef DEBUG
    ff_console_print("AAAA: FFAudioPlayer::Destructor");
#endif

    delete reinterpret_cast<FFAudioPlayer*>(nativeObject);
}

napi_value FFAudioPlayer::New(napi_env env, napi_callback_info info) {
#ifdef DEBUG
    ff_console_print("AAAA: FFAudioPlayer::New");
#endif

    napi_value new_target;
    napi_get_new_target(env, info, &new_target);
    if ( new_target == nullptr ) {
        napi_throw_error(env, nullptr, "FFAudioPlayer must be called with 'new'");
        return nullptr;
    }

    napi_value js_this;
    napi_get_cb_info(env, info, nullptr, nullptr, &js_this, nullptr);
    
    // 创建 C++ 对象 
    FFAudioPlayer* obj = new FFAudioPlayer();

    // 通过 napi_wrap 将 ArkTS 对象 this 与 C++ 对象 obj 绑定
    napi_wrap(env, js_this, reinterpret_cast<void*>(obj), FFAudioPlayer::Destructor, nullptr, nullptr);
    return js_this;
}

// Utils
static napi_value ToJsError(napi_env env, const FFAV::Error* error) {
    napi_value result;
    if ( error ) {
        // 构造errorCode和errorMessage
        napi_value errorCode, errorMessage;
        napi_create_string_utf8(env, "FF_PLAYBACK_ERR", NAPI_AUTO_LENGTH, &errorCode);
        napi_create_string_utf8(env, error->msg.c_str(), NAPI_AUTO_LENGTH, &errorMessage);
        // 调用napi_create_type_error创建一个typeError错误对象
        napi_create_type_error(env, errorCode, errorMessage, &result);
    }
    else {
        napi_get_undefined(env, &result);
    }
    return result;
}

static std::string NapiValueToString(napi_env env, napi_value value) {
    size_t str_size = 0;
    napi_get_value_string_utf8(env, value, nullptr, 0, &str_size);
    
    std::string result(str_size, '\0');
    napi_get_value_string_utf8(env, value, &result[0], str_size + 1, &str_size);
    return result;
}

napi_value FFAudioPlayer::GetUrl(napi_env env, napi_callback_info info) {
#ifdef DEBUG
    ff_console_print("AAAA: FFAudioPlayer::GetUrl");
#endif

    napi_value js_this;
    napi_get_cb_info(env, info, nullptr, nullptr, &js_this, nullptr);

    FFAudioPlayer* obj;
    napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));

    napi_value result;
    if ( !obj->url.empty() ) {
        napi_create_string_utf8(env, obj->url.c_str(), NAPI_AUTO_LENGTH, &result);
    }
    else {
        napi_get_undefined(env, &result);
    }
    return result;
}

napi_value FFAudioPlayer::SetUrl(napi_env env, napi_callback_info info) {
#ifdef DEBUG
    ff_console_print("AAAA: FFAudioPlayer::SetUrl");
#endif

    size_t argc = 2;
    int url_idx = 0;
    int opts_idx = 1;

    napi_value args[argc];
    napi_value js_this;

    napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

    FFAudioPlayer* obj;
    napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
    
    obj->url.clear();
    obj->options.reset();
    
    napi_valuetype urltype;
    napi_typeof(env, args[url_idx], &urltype);

    if ( urltype == napi_null || urltype == napi_undefined ) {
        return nullptr;
    }

    size_t url_size = 0;
    napi_get_value_string_utf8(env, args[url_idx], nullptr, 0, &url_size);
    if ( url_size == 0 ) {
        napi_throw_error(env, nullptr, "'url' can't be an empty string");
        return nullptr;
    }

    std::string new_url(url_size, '\0'); 
    napi_get_value_string_utf8(env, args[url_idx], &new_url[0], url_size + 1, &url_size);
    obj->url = std::move(new_url);  
    
    
    int64_t start_time_position_ms = 0;
    std::map<std::string, std::string> http_options;
    OH_AudioStream_Usage stream_usage = AUDIOSTREAM_USAGE_MUSIC;

    napi_value opts = args[opts_idx];
    napi_valuetype valuetype;
    napi_typeof(env, opts, &valuetype);
    if ( valuetype != napi_undefined ) {
        napi_value opt;
        napi_get_named_property(env, opts, "startTimePosition", &opt);
        napi_typeof(env, opt, &valuetype);
        if ( valuetype != napi_undefined ) {
            napi_get_value_int64(env, opt, &start_time_position_ms);
        }
        
        napi_get_named_property(env, opts, "httpOptions", &opt);
        napi_typeof(env, opt, &valuetype);
        if ( valuetype != napi_undefined ) {
            // 获取对象的所有键
            napi_value keys;
            napi_get_property_names(env, opt, &keys);
            // 获取键的数量
            uint32_t length = 0;
            napi_get_array_length(env, keys, &length);
            
            for (uint32_t i = 0; i < length; i++) {
                napi_value key, value;
                napi_get_element(env, keys, i, &key);
                std::string keyStr = NapiValueToString(env, key);
                napi_get_named_property(env, opt, keyStr.c_str(), &value);
                std::string valueStr = NapiValueToString(env, value);
                http_options[keyStr] = valueStr;
            }
        }
        
        napi_get_named_property(env, opts, "streamUsage", &opt);
        napi_typeof(env, opt, &valuetype);
        if ( valuetype != napi_undefined ) {
            napi_get_value_int32(env, opt, (int32_t *)&stream_usage);
        }
    }
    
    obj->options.start_time_position_ms = start_time_position_ms;
    obj->options.http_options = std::move(http_options);
    obj->options.stream_usage = stream_usage;
    return nullptr;
}

napi_value FFAudioPlayer::Prepare(napi_env env, napi_callback_info info) {
    napi_value js_this;
    napi_get_cb_info(env, info, nullptr, nullptr, &js_this, nullptr);

    FFAudioPlayer* obj;
    napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
    
    obj->prepare();
    return nullptr;
}

napi_value FFAudioPlayer::Play(napi_env env, napi_callback_info info) {
    napi_value js_this;
    napi_get_cb_info(env, info, nullptr, nullptr, &js_this, nullptr);

    FFAudioPlayer* obj;
    napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
    
    obj->play();
    return nullptr;
}

napi_value FFAudioPlayer::Pause(napi_env env, napi_callback_info info) {
    napi_value js_this;
    napi_get_cb_info(env, info, nullptr, nullptr, &js_this, nullptr);

    FFAudioPlayer* obj;
    napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
    
    obj->pause();
    return nullptr;
}

napi_value FFAudioPlayer::Stop(napi_env env, napi_callback_info info) {
    napi_value js_this;
    napi_get_cb_info(env, info, nullptr, nullptr, &js_this, nullptr);

    FFAudioPlayer* obj;
    napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
    
    obj->stop();
    return nullptr;
}

napi_value FFAudioPlayer::Seek(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    int time_idx = 0;

    napi_value args[argc];
    napi_value js_this;

    napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);
    
    int64_t time_ms;
    napi_get_value_int64(env, args[time_idx], &time_ms);

    FFAudioPlayer* obj;
    napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));

    obj->seek(time_ms);
    return nullptr;
}

napi_value FFAudioPlayer::GetVolume(napi_env env, napi_callback_info info) {
    napi_value js_this;
    napi_get_cb_info(env, info, nullptr, nullptr, &js_this, nullptr);

    FFAudioPlayer* obj;
    napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));

    napi_value volume_value;
    napi_create_double(env, obj->volume, &volume_value);
    return volume_value;
}

napi_value FFAudioPlayer::SetVolume(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    int volume_idx = 0;

    napi_value args[argc];
    napi_value js_this;

    napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);
    
    double volume;
    napi_get_value_double(env, args[volume_idx], &volume);

    FFAudioPlayer* obj;
    napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
    
    obj->setVolume(volume);
    return nullptr;
}

napi_value FFAudioPlayer::GetSpeed(napi_env env, napi_callback_info info) {
    napi_value js_this;
    napi_get_cb_info(env, info, nullptr, nullptr, &js_this, nullptr);

    FFAudioPlayer* obj;
    napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));

    napi_value speed_value;
    napi_create_double(env, obj->speed, &speed_value);
    return speed_value;
}

napi_value FFAudioPlayer::SetSpeed(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    int speed_idx = 0;

    napi_value args[argc];
    napi_value js_this;

    napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);
    
    double speed;
    napi_get_value_double(env, args[speed_idx], &speed);

    FFAudioPlayer* obj;
    napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
    
    obj->setSpeed(speed);
    return nullptr;
}

napi_value FFAudioPlayer::SetDefaultOutputDevice(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    int device_type_idx = 0;

    napi_value args[argc];
    napi_value js_this;

    napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);
    
    int32_t device_type;
    napi_get_value_int32(env, args[device_type_idx], &device_type);

    FFAudioPlayer* obj;
    napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
    
    obj->setDeviceType(device_type);
    return nullptr;
}

napi_value FFAudioPlayer::GetPlayWhenReady(napi_env env, napi_callback_info info) {
    napi_value js_this;
    napi_get_cb_info(env, info, nullptr, nullptr, &js_this, nullptr);

    FFAudioPlayer* obj;
    napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
    
    napi_value value;
    napi_get_boolean(env, obj->play_when_ready.load(), &value);
    return value;
}

napi_value FFAudioPlayer::GetCurrentTime(napi_env env, napi_callback_info info) {
    napi_value js_this;
    napi_get_cb_info(env, info, nullptr, nullptr, &js_this, nullptr);

    FFAudioPlayer* obj;
    napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
    
    napi_value time_ms;
    napi_create_int64(env, obj->current_time_ms.load(), &time_ms);
    return time_ms;
}

napi_value FFAudioPlayer::GetDuration(napi_env env, napi_callback_info info) {
    napi_value js_this;
    napi_get_cb_info(env, info, nullptr, nullptr, &js_this, nullptr);

    FFAudioPlayer* obj;
    napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
    
    napi_value time_ms;
    napi_create_int64(env, obj->duration_ms.load(), &time_ms);
    return time_ms;
}

napi_value FFAudioPlayer::GetPlayableDuration(napi_env env, napi_callback_info info) {
    napi_value js_this;
    napi_get_cb_info(env, info, nullptr, nullptr, &js_this, nullptr);

    FFAudioPlayer* obj;
    napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
    
    napi_value time_ms;
    napi_create_int64(env, obj->playable_duration_ms.load(), &time_ms);
    return time_ms;
}

napi_value FFAudioPlayer::GetError(napi_env env, napi_callback_info info) {
    napi_value js_this;
    napi_get_cb_info(env, info, nullptr, nullptr, &js_this, nullptr);

    FFAudioPlayer* obj;
    napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
    
    return ToJsError(env, obj->cur_error.load());
}

// js_func_play_when_ready_callback: (play_when_ready, reason) => void
struct PlayWhenReadyChangeData {
    bool play_when_ready;
    PlayWhenReadyChangeReason reason;
}; 

static void InvokeJsPlayWhenReadyChangeCallback(napi_env env, napi_value js_callback, void* context, void* data) {
    PlayWhenReadyChangeData* state = static_cast<PlayWhenReadyChangeData*>(data);
    // 调用 JavaScript 回调
    napi_value undefined, play_when_ready, reason;
    napi_get_undefined(env, &undefined);
    napi_get_boolean(env, state->play_when_ready, &play_when_ready);
    napi_create_int32(env, static_cast<int32_t>(state->reason), &reason);
    napi_value args[] = { play_when_ready, reason };
    napi_call_function(env, undefined, js_callback, 2, args, nullptr);
    delete state;
}

// js_func_duration_change_callback: (duration_ms) => void
// js_func_current_time_change_callback: (cur_time_ms) => void
// js_func_playable_duration_change_callback: (playable_duration_ms) => void
static void InvokeJsTimeChangeCallback(napi_env env, napi_value js_callback, void* context, void* data) {
    int64_t* time = static_cast<int64_t*>(data);
    // 调用 JavaScript 回调
    napi_value undefined, result;
    napi_get_undefined(env, &undefined);
    napi_create_int64(env, *time, &result);
    napi_call_function(env, undefined, js_callback, 1, &result, nullptr);
    delete time;
}

// js_func_error_callback: (error) => void
static void InvokeJsErrorChangeCallback(napi_env env, napi_value js_callback, void* context, void* data) {
    if ( data ) {
        FFAV::Error* original_err = static_cast<FFAV::Error*>(data);
        napi_value undefined, error;
        napi_get_undefined(env, &undefined);
        error = ToJsError(env, original_err);
        napi_call_function(env, undefined, js_callback, 1, &error, nullptr);
    }
    else {
        napi_value undefined;
        napi_get_undefined(env, &undefined);
        napi_call_function(env, undefined, js_callback, 1, &undefined, nullptr);
    }
}

napi_value FFAudioPlayer::On(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    int event_name_idx = 0;
    int js_func_idx = 1;

    napi_value args[argc];
    napi_value js_this;
    napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

    // 获取 event_name
    size_t event_name_size = 0;
    napi_get_value_string_utf8(env, args[event_name_idx], nullptr, 0, &event_name_size);
    std::string event_name(event_name_size, '\0');
    napi_get_value_string_utf8(env, args[event_name_idx], &event_name[0], event_name_size + 1, &event_name_size);

    FFAudioPlayer* obj;
    napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));

    napi_threadsafe_function* js_func = nullptr;
    napi_threadsafe_function_call_js invoke_func = nullptr;
    if ( event_name == "playWhenReadyChange" ) {
        js_func = &obj->js_func_play_when_ready_callback;
        invoke_func = InvokeJsPlayWhenReadyChangeCallback;
    }
    else if ( event_name == "durationChange" ) {
        js_func = &obj->js_func_duration_change_callback;
        invoke_func = InvokeJsTimeChangeCallback; 
    }
    else if ( event_name == "currentTimeChange" ) {
        js_func = &obj->js_func_current_time_change_callback;
        invoke_func = InvokeJsTimeChangeCallback; 
    }
    else if ( event_name == "playableDurationChange" ) {
        js_func = &obj->js_func_playable_duration_change_callback;
        invoke_func = InvokeJsTimeChangeCallback; 
    }
    else if ( event_name == "errorChange" ) {
        js_func = &obj->js_func_error_change_callback;
        invoke_func = InvokeJsErrorChangeCallback;
    }
    
    if ( js_func != nullptr && invoke_func != nullptr ) {
        if ( *js_func ) {
            napi_release_threadsafe_function(*js_func, napi_tsfn_release);
            *js_func = nullptr;
        }

        napi_value async_resource_name;
        napi_create_string_utf8(env, event_name.c_str(), NAPI_AUTO_LENGTH, &async_resource_name);
        napi_create_threadsafe_function(env, args[js_func_idx], nullptr, async_resource_name, 0, 1, nullptr, nullptr, nullptr, invoke_func, js_func);
    }
    return nullptr;
}

napi_value FFAudioPlayer::Off(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    int event_name_idx = 0;

    napi_value args[argc];
    napi_value js_this;
    napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

    // 获取 event_name
    size_t event_name_size = 0;
    napi_get_value_string_utf8(env, args[event_name_idx], nullptr, 0, &event_name_size);
    std::string event_name(event_name_size, '\0');
    napi_get_value_string_utf8(env, args[event_name_idx], &event_name[0], event_name_size + 1, &event_name_size);

    FFAudioPlayer* obj;
    napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
        
    napi_threadsafe_function* js_func = nullptr;
    if ( event_name == "playWhenReadyChange" ) {
        js_func = &obj->js_func_play_when_ready_callback;
    }
    else if ( event_name == "durationChange" ) {
        js_func = &obj->js_func_duration_change_callback;
    }
    else if ( event_name == "currentTimeChange" ) {
        js_func = &obj->js_func_current_time_change_callback;
    }
    else if ( event_name == "playableDurationChange" ) {
        js_func = &obj->js_func_playable_duration_change_callback;
    }
    else if ( event_name == "errorChange" ) {
        js_func = &obj->js_func_error_change_callback;
    }

    if ( js_func && *js_func ) {
        napi_release_threadsafe_function(*js_func, napi_tsfn_release);
        *js_func = nullptr;
    }
    return nullptr;
}

FFAudioPlayer::FFAudioPlayer() = default;

FFAudioPlayer::~FFAudioPlayer() {
#ifdef DEBUG
    ff_console_print("AAAA: FFAudioPlayer::~FFAudioPlayer");
#endif

    if ( js_func_duration_change_callback ) {
        napi_release_threadsafe_function(js_func_duration_change_callback, napi_tsfn_release);
        js_func_duration_change_callback = nullptr;
    }
    if ( js_func_current_time_change_callback ) {
        napi_release_threadsafe_function(js_func_current_time_change_callback, napi_tsfn_release);
        js_func_current_time_change_callback = nullptr;
    }
    if ( js_func_playable_duration_change_callback ) {
        napi_release_threadsafe_function(js_func_playable_duration_change_callback, napi_tsfn_release);
        js_func_playable_duration_change_callback = nullptr;
    }
    if ( js_func_error_change_callback ) {
        napi_release_threadsafe_function(js_func_error_change_callback, napi_tsfn_release);
        js_func_error_change_callback = nullptr;
    }
    FFAV::Error* error = cur_error.load();
    if ( error ) {
        delete error;
    }
}

bool FFAudioPlayer::createPlayer() {
    if ( player == nullptr && !url.empty() ) {
        player = new FFAV::AudioPlayer(url, options);
        if ( speed != 1 ) player->setSpeed(speed);
        if ( volume != 1 ) player->setVolume(volume);
        if ( device_type != OH_AudioDevice_Type::AUDIO_DEVICE_TYPE_DEFAULT ) player->setDefaultOutputDevice(device_type);
        player->setEventCallback(std::bind(&FFAudioPlayer::onPlayerEvent, this, std::placeholders::_1));
    }
    return player != nullptr;
}

void FFAudioPlayer::prepare() {
    if ( createPlayer() ) {
        player->prepare();
        if ( this->play_when_ready.load() ) player->play();
    }
}

void FFAudioPlayer::play() {
    if ( cur_error.load() ) {
        return;
    }
    onPlayWhenReadyChange(true, FFAV::PlayWhenReadyChangeReason::USER_REQUEST);

    if ( createPlayer() ) {
        player->play();
    }
}

void FFAudioPlayer::pause() {
    if ( cur_error.load() ) {
        return;
    }
    onPlayWhenReadyChange(false, FFAV::PlayWhenReadyChangeReason::USER_REQUEST);

    if ( player != nullptr ) {
        player->pause();
    }
}

void FFAudioPlayer::stop() {
    if ( player != nullptr ) {
        delete player;
        player = nullptr;
    }
    
    onErrorChange(nullptr);
    onCurrentTimeChange(0);
    onPlayableDurationChange(0);
    onDurationChange(0);
    onPlayWhenReadyChange(false, FFAV::PlayWhenReadyChangeReason::USER_REQUEST);
    
    url.clear();
    options.reset();
}

void FFAudioPlayer::seek(int64_t time_ms) {
    if ( cur_error.load() ) {
        return;
    }    

    if ( player != nullptr ) {
        player->seek(time_ms);
    }
}

void FFAudioPlayer::setVolume(float volume) {
    if ( volume < 0 ) volume = 0;
    else if ( volume > 1 ) volume = 1;
    if ( this->volume != volume ) {
        this->volume = volume;
        if ( player ) {
            player->setVolume(volume);
        }
    }
}

void FFAudioPlayer::setSpeed(float speed) {
    if ( speed < 0.25 ) speed = 0.25;
    else if ( speed > 4 ) speed = 4;
    if ( this->speed != speed ) {
        this->speed = speed;
        if ( player ) {
            player->setSpeed(speed);
        }
    }
}

void FFAudioPlayer::setDeviceType(int32_t device_type) {
    if ( this->device_type != device_type ) {
        this->device_type = (OH_AudioDevice_Type)device_type;
        if ( player ) {
            player->setDefaultOutputDevice(this->device_type);
        }
    }
}

void FFAudioPlayer::onPlayerEvent(std::shared_ptr<FFAV::EventMessage> msg_ptr) {
    auto msg = msg_ptr.get();
    switch(msg->type) {
    case FFAV::EventType::MSG_PLAY_WHEN_READY_CHANGE: {
        auto change_msg = static_cast<const FFAV::PlayWhenReadyChangeEventMessage *>(msg); 
        onPlayWhenReadyChange(change_msg->play_when_ready, change_msg->reason);
    }
        break;
    case FFAV::EventType::MSG_DURATION_CHANGE: {
        onDurationChange(static_cast<const FFAV::DurationChangeEventMessage*>(msg)->duration);
    }
        break;
    case FFAV::EventType::MSG_CURRENT_TIME_CHANGE: {
        onCurrentTimeChange(static_cast<const FFAV::CurrentTimeEventMessage*>(msg)->current_time);
    }
        break;
    case FFAV::EventType::MSG_PLAYABLE_DURATION_CHANGE: {
        onPlayableDurationChange(static_cast<const FFAV::PlayableDurationChangeEventMessage*>(msg)->playable_duration);
    }
        break;
    case FFAV::EventType::MSG_ERROR: {
        const FFAV::ErrorEventMessage* err_msg = static_cast<const FFAV::ErrorEventMessage*>(msg);
        onErrorChange(err_msg->error->copy());
    }
        break;
    }
}

void FFAudioPlayer::onPlayWhenReadyChange(bool play_when_ready, PlayWhenReadyChangeReason reason) {
 #ifdef DEBUG
    ff_console_print("AAAA: FFAudioPlayer::onPlayWhenReadyChange(%d)", play_when_ready);
#endif
    this->play_when_ready.store(play_when_ready);

    if ( js_func_play_when_ready_callback ) {
        PlayWhenReadyChangeData* data = new PlayWhenReadyChangeData { play_when_ready, reason };
        napi_call_threadsafe_function(js_func_play_when_ready_callback, data, napi_tsfn_nonblocking);
    }
}

void FFAudioPlayer::onDurationChange(int64_t duration_ms) {
#ifdef DEBUG
    ff_console_print("AAAA: FFAudioPlayer::onDurationChange(%lld)", duration_ms);
#endif

    this->duration_ms.store(duration_ms);

    if ( js_func_duration_change_callback ) {
        int64_t* time = new int64_t(duration_ms);
        napi_call_threadsafe_function(js_func_duration_change_callback, time, napi_tsfn_nonblocking);
    }
}

void FFAudioPlayer::onCurrentTimeChange(int64_t current_time_ms) {
#ifdef DEBUG
    ff_console_print("AAAA: FFAudioPlayer::onCurrentTimeChange(%lld)", current_time_ms);
#endif

    this->current_time_ms.store(current_time_ms);

    if ( js_func_current_time_change_callback ) {
        int64_t* time = new int64_t(current_time_ms);
        napi_call_threadsafe_function(js_func_current_time_change_callback, time, napi_tsfn_nonblocking);
    }
}

void FFAudioPlayer::onPlayableDurationChange(int64_t playable_duration_ms) {
#ifdef DEBUG
    ff_console_print("AAAA: FFAudioPlayer::onPlayableDurationChange(%lld)", playable_duration_ms);
#endif

    this->playable_duration_ms.store(playable_duration_ms);

    if ( js_func_playable_duration_change_callback ) {
        int64_t* time = new int64_t(playable_duration_ms);
        napi_call_threadsafe_function(js_func_playable_duration_change_callback, time, napi_tsfn_nonblocking);
    }    
}

void FFAudioPlayer::onErrorChange(FFAV::Error* error) {
#ifdef DEBUG
    ff_console_print("AAAA: FFAudioPlayer::onErrorChange(%ld, %s)", error->code, error->msg.c_str());
#endif
    
    FFAV::Error* c_e = cur_error.load();
    if ( c_e ) {
        delete c_e;
    }

    cur_error.store(error);

    if ( js_func_error_change_callback ) {
        napi_call_threadsafe_function(js_func_error_change_callback, error ? error->copy() : nullptr, napi_tsfn_nonblocking);
    }
}

}