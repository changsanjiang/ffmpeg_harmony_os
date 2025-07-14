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
// Created on 2025/2/25.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef FFAV_AudioPlayer_hpp
#define FFAV_AudioPlayer_hpp

#include <stdint.h>
#include <memory>
#include "EventMessageQueue.h"
#include "ff_audio_renderer.hpp"
#include "ff_audio_const.hpp"
#include "av/ffwrap/ff_audio_item.hpp"

namespace FFAV {

class AudioItem;

class AudioPlayer {
public:
    AudioPlayer(const std::string& url, const AudioPlaybackOptions& options);
    virtual ~AudioPlayer();

    void prepare();
    void play();
    void pause();
    void seek(int64_t time_pos_ms);
    
    // [0.0, 1.0]
    void setVolume(float volume);
    // [0.25, 4.0]
    void setSpeed(float speed);
    
    void setDefaultOutputDevice(OH_AudioDevice_Type device_type);
    
    void setEventCallback(EventMessageQueue::EventCallback callback);
    
    int64_t getDurationPlayed() const; // 返回毫秒;
    
protected:
    virtual AudioItem* onCreateAudioItem(const std::string& url, const AudioItem::Options& options);
    AudioItem* getAudioItem();
    std::mutex mtx;
    
private:
    void onFFmpegError(int ff_err);
    void onRenderError(OH_AudioStream_Result render_err);
    void onError(std::shared_ptr<Error> error);
    void onPrepare();
    
    void onPlay(PlayWhenReadyChangeReason reason);
    void onPause(PlayWhenReadyChangeReason reason, bool should_invoke_pause = true);
    
    void onEvent(std::shared_ptr<EventMessage> msg);
    
    OH_AudioData_Callback_Result onRendererWriteDataCallback(void* audio_buffer, int audio_buffer_size_in_bytes);
    void onRendererInterruptEventCallback(OH_AudioInterrupt_ForceType type, OH_AudioInterrupt_Hint hint);
    void onRendererErrorCallback(OH_AudioStream_Result error);
    void onOutputDeviceChangeCallback(OH_AudioStream_DeviceChangeReason reason);
    
    void startRenderer();
    
private:
    std::string _url;
    AudioPlaybackOptions _options;
    
    int _output_sample_rate; 
    AVSampleFormat _output_sample_format;
    int _output_channels;
    int _output_bytes_per_sample;
    
    AudioItem* _audio_item { nullptr };
    AudioRenderer* _audio_renderer { nullptr };
    
    float _volume { 1 };
    float _speed { 1 };
    
    OH_AudioDevice_Type _device_type { OH_AudioDevice_Type::AUDIO_DEVICE_TYPE_DEFAULT };
    
    int64_t _duration_ms { 0 };
    std::atomic<int64_t> _duration_played { 0 }; // accumulated play time: 累计播放的时间，in output time base;
    
    PlayWhenReadyChangeReason _play_when_ready_change_reason = USER_REQUEST;
    
    EventMessageQueue* _event_msg_queue = new EventMessageQueue();
    
    struct {
        unsigned released :1;
        unsigned prepared :1;
        unsigned has_error :1;
        
        unsigned play_when_ready :1;
        unsigned is_renderer_running :1;
        unsigned is_playback_ended :1;
    } _flags = { 0 };
};

}

#endif //FFAV_AudioPlayer_hpp
