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

#include "AudioPlayer.h"
#include "av/ohutils/OHUtils.hpp"
#include <cerrno>
#include <stdint.h>
#include "../ffwrap/ff_audio_item.hpp"
#include "../utils/logger.h"

namespace FFAV {

const int OUTPUT_SAMPLE_RATE = 44100;
const AVSampleFormat OUTPUT_SAMPLE_FORMAT = AV_SAMPLE_FMT_S16;
const int OUTPUT_CHANNELS = 2;

AudioPlayer::AudioPlayer(const std::string& url, const AudioPlaybackOptions& options): stream_usage(options.stream_usage) {
    AudioItem::Options opts;
    if ( options.start_time_position_ms > 0 ) {
        opts.start_time_pos = av_rescale_q(options.start_time_position_ms, (AVRational){ 1, 1000 }, AV_TIME_BASE_Q);
    }
    opts.http_options = options.http_options;
    opts.output_sample_rate = OUTPUT_SAMPLE_RATE;
    opts.output_sample_format = OUTPUT_SAMPLE_FORMAT;
    opts.output_channels = OUTPUT_CHANNELS;
    audio_item = new AudioItem(url, opts);
    audio_renderer = new AudioRenderer();
}

AudioPlayer::~AudioPlayer() {
#ifdef DEBUG
    ff_console_print("AAAA: AudioPlayer::~AudioPlayer before");
#endif
    {
        std::lock_guard<std::mutex> lock(mtx);
        flags.released = true;
    }
    
    event_msg_queue->stop();
    delete event_msg_queue;
    
    if ( audio_renderer ) {
        audio_renderer->stop();
        delete audio_renderer;
    }

    if ( audio_item ) {
        delete audio_item;
    }
    
#ifdef DEBUG
    ff_console_print("AAAA: AudioPlayer::~AudioPlayer after");
#endif
}

void AudioPlayer::prepare() {
    std::lock_guard<std::mutex> lock(mtx);
    onPrepare();
}

void AudioPlayer::play() {
    std::lock_guard<std::mutex> lock(mtx);
    onPlay(PlayWhenReadyChangeReason::USER_REQUEST);
}

void AudioPlayer::pause() {
    std::lock_guard<std::mutex> lock(mtx);
    onPause(PlayWhenReadyChangeReason::USER_REQUEST);
}

void AudioPlayer::seek(int64_t time_pos_ms) {
    std::unique_lock<std::mutex> lock(mtx);
    if ( !flags.prepared || flags.has_error || flags.released ) {
        return;
    }
    
    flags.is_playback_ended = false;
    audio_item->seekTo(av_rescale_q(time_pos_ms, (AVRational){ 1, 1000 }, AV_TIME_BASE_Q));
}

void AudioPlayer::setVolume(float volume) {
    std::lock_guard<std::mutex> lock(mtx);
    if ( flags.has_error || flags.released ) {
        return;
    }
    
    if ( volume < 0.0f ) volume = 0.0f;
    else if ( volume > 1.0f ) volume = 1.0f;
    
    this->volume = volume;
    
    if ( flags.prepared ) {
        audio_renderer->setVolume(volume);
    }
}

void AudioPlayer::setSpeed(float speed) {
    std::lock_guard<std::mutex> lock(mtx);
    if ( flags.has_error || flags.released ) {
        return;
    }
    
    if ( speed < 0.25f ) speed = 0.25f;
    else if ( speed > 4.0f ) speed = 4.0f;
    
    this->speed = speed;
    
    if ( flags.prepared ) {
        audio_renderer->setSpeed(speed);
    }
}

void AudioPlayer::setDefaultOutputDevice(OH_AudioDevice_Type device_type) {
    std::lock_guard<std::mutex> lock(mtx);
    this->device_type = device_type;
    
    if ( flags.prepared ) {
        audio_renderer->setDefaultOutputDevice(device_type);
    }
}

void AudioPlayer::setEventCallback(EventMessageQueue::EventCallback callback) {
    event_msg_queue->setEventCallback(callback);
}

void AudioPlayer::onPrepare() {
    if ( flags.prepared || flags.has_error || flags.released ) {
        return;
    }
    
    // init audio item
    audio_item->setStreamReadyCallback([&](int64_t duration, AVRational time_base) {
        duration_ms = av_rescale_q(duration, time_base, (AVRational){ 1, 1000 });
        onEvent(std::make_shared<DurationChangeEventMessage>(duration_ms)); 
    });
    audio_item->setBufferedTimeChangeCallback([&](int64_t buffered_time, AVRational time_base) {
        onEvent(std::make_shared<PlayableDurationChangeEventMessage>(av_rescale_q(buffered_time, time_base, (AVRational){ 1, 1000 })));
    });
    audio_item->setErrorCallback([&](int ff_err) {
        onFFmpegError(ff_err);
    });
    
    output_nb_channels = audio_item->getOutputChannels();
    output_nb_bytes_per_sample = av_get_bytes_per_sample(audio_item->getOutputSampleFormat());
    
    // init audio renderer
    OH_AudioStream_Result render_ret = audio_renderer->init(FFAV::Conversion::toOHFormat(audio_item->getOutputSampleFormat()), audio_item->getOutputSampleRate(), audio_item->getOutputChannels(), stream_usage);
    if ( render_ret != AUDIOSTREAM_SUCCESS ) {
        onRenderError(render_ret);
        return;
    }
    
    if ( volume != 1 ) audio_renderer->setVolume(volume);
    if ( speed != 1 ) audio_renderer->setSpeed(speed);
    if ( device_type != OH_AudioDevice_Type::AUDIO_DEVICE_TYPE_DEFAULT ) audio_renderer->setDefaultOutputDevice(device_type);
    
    // set callbacks
    audio_renderer->setWriteDataCallback(std::bind(&AudioPlayer::onRendererWriteDataCallback, this, std::placeholders::_1, std::placeholders::_2));
    audio_renderer->setInterruptEventCallback(std::bind(&AudioPlayer::onRendererInterruptEventCallback, this, std::placeholders::_1, std::placeholders::_2));
    audio_renderer->setErrorCallback(std::bind(&AudioPlayer::onRendererErrorCallback, this, std::placeholders::_1));
    audio_renderer->setOutputDeviceChangeCallback(std::bind(&AudioPlayer::onOutputDeviceChangeCallback, this, std::placeholders::_1));

    // prepare
    audio_item->prepare();
    
    flags.prepared = true;
}

OH_AudioData_Callback_Result AudioPlayer::onRendererWriteDataCallback(void* write_buffer, int write_buffer_size_in_bytes) {
    std::unique_lock<std::mutex> lock(mtx);
    
    int capacity = write_buffer_size_in_bytes / output_nb_bytes_per_sample / output_nb_channels;
    int64_t pts = 0;
    bool eof = false;
    int ret = audio_item->tryTranscode(&write_buffer, capacity, &pts, &eof);
    
    int samples_read = ret > 0 ? ret : 0;
    if ( samples_read < capacity ) {
        int bytes_read = samples_read * output_nb_channels * output_nb_bytes_per_sample;
        memset(static_cast<uint8_t*>(write_buffer) + bytes_read, 0, write_buffer_size_in_bytes - bytes_read);
    }
    
    // playback ended
    if ( samples_read == 0 && eof ) {
        flags.is_playback_ended = true;
        onEvent(std::make_shared<CurrentTimeEventMessage>(duration_ms));
        onPause(PlayWhenReadyChangeReason::PLAYBACK_ENDED);
    }
    // error
    else if ( ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF ) {
        onFFmpegError(ret);
    }
    else if ( samples_read > 0 ) {
        onEvent(std::make_shared<CurrentTimeEventMessage>(av_rescale_q(pts, (AVRational){ 1, audio_item->getOutputSampleRate() }, (AVRational){ 1, 1000 })));
    }
    return AUDIO_DATA_CALLBACK_RESULT_VALID;
}

void AudioPlayer::onRendererInterruptEventCallback(OH_AudioInterrupt_ForceType type, OH_AudioInterrupt_Hint hint) {
    std::unique_lock<std::mutex> lock(mtx);
    // 处理音频焦点变化
    // https://developer.huawei.com/consumer/cn/doc/harmonyos-guides-V13/audio-playback-concurrency-V13#处理音频焦点变化
    
    // 在发生音频焦点变化时，audioRenderer收到interruptEvent回调，此处根据其内容做相应处理
    // 1. 可选：读取interruptEvent.forceType的类型，判断系统是否已强制执行相应操作。
    // 注：默认焦点策略下，INTERRUPT_HINT_RESUME为INTERRUPT_SHARE类型，其余hintType均为INTERRUPT_FORCE类型。因此对forceType可不做判断。
    // 2. 必选：读取interruptEvent.hintType的类型，做出相应的处理。
    switch (type) {
        // 强制打断类型（INTERRUPT_FORCE）：音频相关处理已由系统执行，应用需更新自身状态，做相应调整
        case AUDIOSTREAM_INTERRUPT_FORCE: {
            switch (hint) {
                case AUDIOSTREAM_INTERRUPT_HINT_NONE:
                // 强制打断类型下不会进入这个分支;
                // 提示音频恢复，应用可主动触发开始渲染或开始采集的相关操作。
                // 此操作无法由系统强制执行，其对应的OH_AudioInterrupt_ForceType一定为AUDIOSTREAM_INTERRUPT_SHARE类型。
                case AUDIOSTREAM_INTERRUPT_HINT_RESUME:
                    break;
                case AUDIOSTREAM_INTERRUPT_HINT_PAUSE: {
                    // 此分支表示系统已将音频流暂停（临时失去焦点），为保持状态一致，应用需切换至音频暂停状态
                    // 临时失去焦点：待其他音频流释放音频焦点后，本音频流会收到resume对应的音频焦点事件，到时可自行继续播放
                    onPause(PlayWhenReadyChangeReason::AUDIO_INTERRUPT_PAUSE, false);
                }
                    break;
                case AUDIOSTREAM_INTERRUPT_HINT_STOP: {
                    // 此分支表示系统已将音频流停止（永久失去焦点），为保持状态一致，应用需切换至音频暂停状态
                    // 永久失去焦点：后续不会再收到任何音频焦点事件，若想恢复播放，需要用户主动触发。
                    onPause(PlayWhenReadyChangeReason::AUDIO_INTERRUPT_STOP, false);
                }
                    break;
                case AUDIOSTREAM_INTERRUPT_HINT_DUCK: {
                    // 此分支表示系统已将音频音量降低（默认降到正常音量的20%）
                    // isDucked = true; 
                }
                    break;
                case AUDIOSTREAM_INTERRUPT_HINT_UNDUCK: {
                    // 此分支表示系统已将音频音量恢复正常
                    // isDucked = false; 
                }
                    break;
            }
        }
            break;
        // 共享打断类型（INTERRUPT_SHARE）：应用可自主选择执行相关操作或忽略音频焦点事件
        // 默认焦点策略下，INTERRUPT_HINT_RESUME为INTERRUPT_SHARE类型，其余hintType均为INTERRUPT_FORCE类型。因此对forceType可不做判断。
        case AUDIOSTREAM_INTERRUPT_SHARE: {
            switch (hint) {
                // 此分支表示临时失去焦点后被暂停的音频流此时可以继续播放，建议应用继续播放，切换至音频播放状态
                // 若应用此时不想继续播放，可以忽略此音频焦点事件，不进行处理即可
                // 继续播放，此处主动执行start()，以标识符变量started记录start()的执行结果
            case AUDIOSTREAM_INTERRUPT_HINT_RESUME: {
                if ( play_when_ready_change_reason == PlayWhenReadyChangeReason::AUDIO_INTERRUPT_PAUSE ) {
                    onPlay(PlayWhenReadyChangeReason::AUDIO_INTERRUPT_RESUME);
                }
            }
                break;
            // 共享打断类型 不会有以下分支
            case AUDIOSTREAM_INTERRUPT_HINT_PAUSE:
            case AUDIOSTREAM_INTERRUPT_HINT_STOP:
            case AUDIOSTREAM_INTERRUPT_HINT_DUCK:
            case AUDIOSTREAM_INTERRUPT_HINT_UNDUCK:
            case AUDIOSTREAM_INTERRUPT_HINT_NONE:
                break;
            }
        }
            break;
    }
}

void AudioPlayer::onRendererErrorCallback(OH_AudioStream_Result error) {
    std::unique_lock<std::mutex> lock(mtx);
    onRenderError(error);
}

void AudioPlayer::onOutputDeviceChangeCallback(OH_AudioStream_DeviceChangeReason reason) {
    std::unique_lock<std::mutex> lock(mtx);
    switch (reason) {
        case REASON_UNKNOWN:
        case REASON_NEW_DEVICE_AVAILABLE:
        case REASON_OVERRODE:
            break;
        case REASON_OLD_DEVICE_UNAVAILABLE: {
            onPause(PlayWhenReadyChangeReason::OLD_DEVICE_UNAVAILABLE);
        }
            break;
    }
}

void AudioPlayer::onFFmpegError(int error) {
    ff_console_print("AAAAA: onFFmpegError(%d), %s", error, av_err2str(error));
    
    onError(Error::FFError(error));
}

void AudioPlayer::onRenderError(OH_AudioStream_Result error) {
    ff_console_print("AAAAA: onRenderError(%d)", error);

    onError(Error::RenderError(error));
}

void AudioPlayer::onError(std::shared_ptr<Error> error) {
    if ( flags.has_error || flags.released ) {
        return;
    }
    
    flags.has_error = true;
    
    if ( flags.is_renderer_running ) {
        audio_renderer->stop();
        flags.is_renderer_running = false;
    }
    onEvent(std::make_shared<ErrorEventMessage>(error));
}

void AudioPlayer::onPlay(PlayWhenReadyChangeReason reason) {
    if ( flags.has_error || flags.released ) {
        return;
    }

    play_when_ready_change_reason = reason;
    
    if ( !flags.play_when_ready ) {
        flags.play_when_ready = true;
        
        if ( !flags.prepared ) {
            onPrepare();
        }
        
        if ( flags.prepared ) {
            startRenderer();
        }
    }
    
    onEvent(std::make_shared<PlayWhenReadyChangeEventMessage>(true, reason));
}

void AudioPlayer::onPause(PlayWhenReadyChangeReason reason, bool should_invoke_pause) {
    
    if ( flags.has_error || flags.released ) {
        return;
    }
    
    play_when_ready_change_reason = reason;
    
    if ( flags.play_when_ready ) {
        flags.play_when_ready = false;
        
        if ( flags.is_renderer_running ) {
            // 当被系统强制中断时 render 没有必要执行 pause, 外部可以传递该参数决定是否需要调用render的暂停;
            if ( should_invoke_pause ) {
              OH_AudioStream_Result render_ret = audio_renderer->pause();
                if ( render_ret != AUDIOSTREAM_SUCCESS ) {
                    onRenderError(render_ret);
                    return;
                }
            }
            
            flags.is_renderer_running = false;
        }
    }
    
    onEvent(std::make_shared<PlayWhenReadyChangeEventMessage>(false, reason));
}

void AudioPlayer::onEvent(std::shared_ptr<EventMessage> msg) {
    event_msg_queue->push(msg);
}

void AudioPlayer::startRenderer() {
    OH_AudioStream_Result render_ret = audio_renderer->play();
    if ( render_ret != AUDIOSTREAM_SUCCESS ) {
        onRenderError(render_ret);
        return;
    }
    flags.is_renderer_running = true;
}

}