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
// Created on 2025/2/9.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef FFMPEGPROJ_AUDIOPLAYER_H
#define FFMPEGPROJ_AUDIOPLAYER_H

#include "EventMessage.h"
#include "Error.h"
#include "av/audio/EventMessageQueue.h"
#include "av/core/MediaReader.h"
#include "av/core/MediaDecoder.h"
#include "av/core/FilterGraph.h"
#include "av/core/AudioRenderer.h"
#include "av/core/AudioFifo.h"
#include "av/core/PacketQueue.h"
#include <cstdint>
#include <string>
#include <thread>
#include <memory>

namespace FFAV {

class AudioPlayer {
public:
    AudioPlayer(const std::string& url);
    ~AudioPlayer();

    void prepare();
    void play();
    void pause();
    void seek(int64_t time_pos_ms);
    
    // [0.0, 1.0]
    void setVolume(float volume);
    // [0.25, 4.0]
    void setSpeed(float speed);
    
    void setEventCallback(EventMessageQueue::EventCallback callback);
    
private:
    const std::string url = nullptr;
    MediaReader* audio_reader = nullptr;
    MediaDecoder* audio_decoder = nullptr;
    FilterGraph* filter_graph = nullptr;
    AudioFifo* audio_fifo = nullptr;
    PacketQueue* pkt_queue = nullptr;
    AudioRenderer* audio_renderer = nullptr;

    std::unique_ptr<std::thread> init_thread = nullptr;
    std::unique_ptr<std::thread> read_thread = nullptr;
    std::unique_ptr<std::thread> dec_thread = nullptr;
    
    std::mutex mtx;
    std::condition_variable cv;
    
    AVRational time_base;
    
    int64_t duration_ms = 0;
    int64_t current_time_ms = 0; // last_render_pts_ms
    int64_t playable_duration_ms = 0; // last_pkt_pts_or_end_time_ms
    
    float volume = 1;
    float speed = 1;

    int64_t seek_time; // in baseq
    int64_t cur_seek_time { -1 }; // in baseq
    
    int audio_stream_index;
    int pkt_size_threshold = 5 * 1024 * 1024; // bytes; 5M;
    int frame_threshold; // pcm samples; >= 5s;
    int out_sample_rate;
    int out_nb_channels;
    int out_bytes_per_sample;
    AVSampleFormat out_sample_fmt;
    OH_AudioStream_SampleFormat out_render_sample_fmt;
    std::string out_ch_layout_desc;
    PlayWhenReadyChangeReason play_when_ready_change_reason = USER_REQUEST;
    std::shared_ptr<Error> cur_error = nullptr;
    EventMessageQueue event_msg_queue {};
    
    struct {
        unsigned play_when_ready :1;
        unsigned prepare_invoked :1;
        unsigned release_invoked :1;
        
        unsigned init_successful :1;
        unsigned has_error :1;
        
        unsigned wants_seek :1;
        unsigned is_seeking :1;
        unsigned is_read_eof :1;
        unsigned is_dec_eof :1;
        unsigned is_render_eof :1;
        unsigned is_playback_ended :1;
        
        unsigned is_playing :1;
    } flags = { 0 };
    
    void onRelease(std::unique_lock<std::mutex>& lock);
    
    void InitThread();
    void ReadThread();
    void DecThread();
    void EventThread();
    
    int initDecoder(AVCodecParameters* codecpar);
    int initFilterGraph(
        const char* buf_src_name,
        const char* buf_sink_name,
        AVBufferSrcParameters *buf_src_params, 
        AVSampleFormat out_sample_fmt,
        int out_sample_rate,
        const std::string& out_ch_layout_desc
    );
    int initAudioFifo(
        AVSampleFormat sample_fmt, 
        int nb_channels, 
        int nb_samples
    );
    int initPacketQueue();
    OH_AudioStream_Result initRenderer(
        OH_AudioStream_SampleFormat sample_fmt, 
        int sample_rate,
        int nb_channels
    );
    
    int resetFilterGraph();
    
    void onFFmpegError(int error, std::unique_lock<std::mutex>& lock);
    void onRenderError(OH_AudioStream_Result error, std::unique_lock<std::mutex>& lock);
    void onError(std::shared_ptr<Error> error, std::unique_lock<std::mutex>& lock);
    void onPrepare();
    void onEvaluate();
    
    void onPlay(PlayWhenReadyChangeReason reason);
    void onPause(PlayWhenReadyChangeReason reason, bool should_invoke_pause = true);
    
    void onEvent(std::shared_ptr<EventMessage> msg);
    
    OH_AudioData_Callback_Result onRendererWriteDataCallback(void* audio_buffer, int audio_buffer_size_in_bytes);
    void onRendererInterruptEventCallback(OH_AudioInterrupt_ForceType type, OH_AudioInterrupt_Hint hint);
    void onRendererErrorCallback(OH_AudioStream_Result error);
    void onOutputDeviceChangeCallback(OH_AudioStream_DeviceChangeReason reason);
};

}

#endif //FFMPEGPROJ_AUDIOPLAYER_H
