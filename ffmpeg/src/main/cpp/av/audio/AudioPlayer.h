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

#ifndef FFMPEG_HARMONY_OS_AUDIOPLAYER_H
#define FFMPEG_HARMONY_OS_AUDIOPLAYER_H

#include "av/audio/AudioPlaybackOptions.h"
#include "av/audio/EventMessageQueue.h"
#include "AudioRenderer.h"
#include <stdint.h>
#include <memory>

namespace FFAV {

class AudioItem;

class AudioPlayer {
    public:
    AudioPlayer(const std::string& url, const AudioPlaybackOptions& options);
    ~AudioPlayer();

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
    
private:
    AudioItem* audio_item { nullptr };
    AudioRenderer* audio_renderer { nullptr };
    
    float volume { 1 };
    float speed { 1 };
    
    OH_AudioStream_Usage stream_usage;
    OH_AudioDevice_Type device_type { OH_AudioDevice_Type::AUDIO_DEVICE_TYPE_DEFAULT };
    
    int64_t duration_ms { 0 };
    int output_nb_channels;
    int output_nb_bytes_per_sample;
    
    PlayWhenReadyChangeReason play_when_ready_change_reason = USER_REQUEST;
    
    EventMessageQueue* event_msg_queue = new EventMessageQueue();
    
    std::mutex mtx;
    
    struct {
        unsigned released :1;
        unsigned prepared :1;
        unsigned has_error :1;
        
        unsigned play_when_ready :1;
        unsigned is_renderer_running :1;
        unsigned is_playback_ended :1;
    } flags = { 0 };
    
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
};

}

#endif //FFMPEG_HARMONY_OS_AUDIOPLAYER_H
