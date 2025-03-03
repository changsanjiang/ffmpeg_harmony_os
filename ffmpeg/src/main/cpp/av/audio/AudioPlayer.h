//
// Created on 2025/2/25.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef FFMPEG_HARMONY_OS_AUDIOPLAYER_H
#define FFMPEG_HARMONY_OS_AUDIOPLAYER_H

#include "av/audio/AudioReader.h"
#include "av/audio/AudioDecoder.h"
#include "av/audio/EventMessageQueue.h"
#include "av/core/AudioRenderer.h"
#include "av/core/PacketQueue.h"
#include <stdint.h>
#include <thread>
#include <memory>

namespace FFAV {

class AudioPlayer {
    public:
    AudioPlayer(const std::string& url, int64_t start_time_position_ms = 0);
    ~AudioPlayer();

    void prepare();
    void play();
    void playImmediately(); // 尽快播放
    void pause();
    void seek(int64_t time_pos_ms);
    
    // [0.0, 1.0]
    void setVolume(float volume);
    // [0.25, 4.0]
    void setSpeed(float speed);
    
    void setEventCallback(EventMessageQueue::EventCallback callback);
    
private:
    const std::string url;
    const int64_t start_time_position_ms;
    
    AudioReader* audio_reader { nullptr };
    AudioDecoder* audio_decoder { nullptr };
    AudioRenderer* audio_renderer { nullptr };
    AudioFifo* audio_fifo { nullptr };
    PacketQueue* pkt_queue { nullptr };
    
    std::mutex mtx;
    std::condition_variable cv;
    std::unique_ptr<std::thread> dec_thread { nullptr };
    
    int64_t duration_ms { 0 };
    int64_t current_time_ms { 0 }; // last_render_pts_ms
    int64_t playable_duration_ms { 0 }; // last_pkt_pts_or_end_time_ms
    
    float volume { 1 };
    float speed { 1 };

    int pkt_size_threshold { 5 * 1024 * 1024 }; // bytes; 5M;
    int render_frame_size;
    int maximum_frame_threshold; // pcm samples; 5s;
    int minimum_frame_threshold; // pcm samples; 3s; 播放器为了维持流畅播放所需的最小缓冲数据量
    PlayWhenReadyChangeReason play_when_ready_change_reason = USER_REQUEST;
    std::shared_ptr<Error> cur_error { nullptr };
    EventMessageQueue* event_msg_queue = new EventMessageQueue();
    
    AVRational audio_stream_time_base;
    AVRational output_time_base;
    int output_nb_channels;
    int output_nb_bytes_per_sample;
    
    struct {
        unsigned init_successful :1;
        unsigned prepare_invoked :1;
        unsigned release_invoked :1;
        unsigned has_error :1;
        
        unsigned is_read_eof :1;
        unsigned is_dec_eof :1;
        unsigned is_render_eof :1;
        unsigned should_reset_current_time :1;
        
        unsigned play_when_ready :1;
        unsigned should_play_immediate :1;
        unsigned is_renderer_running :1;
        unsigned should_drain_fifo :1; 
        unsigned is_playback_ended :1;
    } flags = { 0 };
    
    
    void onReaderReadyToReadCallback(AudioReader* reader, AVStream* stream);
    void onReaderReadPacketCallback(AudioReader* reader, AVPacket* pkt, bool should_flush);
    void onReaderErrorCallback(AudioReader* reader, int ff_err);
    
    void DecThread();
    
    void flush();
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
