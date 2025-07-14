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

#include "ff_audio_player.hpp"
#include <cerrno>
#include <stdint.h>
#include "av/ohutils/OHUtils.hpp"
#include "av/utils/logger.h"
#include "av/ffwrap/ff_audio_item.hpp"

namespace FFAV {

const int OUTPUT_SAMPLE_RATE = 44100;
const AVSampleFormat OUTPUT_SAMPLE_FORMAT = AV_SAMPLE_FMT_S16;
const int OUTPUT_CHANNELS = 2;

AudioPlayer::AudioPlayer(const std::string& url, const AudioPlaybackOptions& options): 
    _url(url), 
    _options(options),
    _output_sample_rate(OUTPUT_SAMPLE_RATE),
    _output_sample_format(OUTPUT_SAMPLE_FORMAT),
    _output_channels(OUTPUT_CHANNELS),
    _output_bytes_per_sample(av_get_bytes_per_sample(OUTPUT_SAMPLE_FORMAT))
{

}

AudioPlayer::~AudioPlayer() {
#ifdef DEBUG
    ff_console_print("AAAA: AudioPlayer::~AudioPlayer before");
#endif
    {
        std::lock_guard<std::mutex> lock(mtx);
        _flags.released = true;
    }
    
    _event_msg_queue->stop();
    delete _event_msg_queue;
    
    if ( _audio_renderer ) {
        _audio_renderer->stop();
        delete _audio_renderer;
    }

    if ( _audio_item ) {
        delete _audio_item;
        _audio_item = nullptr;
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
    if ( !_flags.prepared || _flags.has_error || _flags.released ) {
        return;
    }
    
    _flags.is_playback_ended = false;
    _audio_item->seekTo(av_rescale_q(time_pos_ms, (AVRational){ 1, 1000 }, AV_TIME_BASE_Q));
}

void AudioPlayer::setVolume(float volume) {
    std::lock_guard<std::mutex> lock(mtx);
    if ( _flags.has_error || _flags.released ) {
        return;
    }
    
    if ( volume < 0.0f ) volume = 0.0f;
    else if ( volume > 1.0f ) volume = 1.0f;
    
    this->_volume = volume;
    
    if ( _flags.prepared ) {
        _audio_renderer->setVolume(volume);
    }
}

void AudioPlayer::setSpeed(float speed) {
    std::lock_guard<std::mutex> lock(mtx);
    if ( _flags.has_error || _flags.released ) {
        return;
    }
    
    if ( speed < 0.25f ) speed = 0.25f;
    else if ( speed > 4.0f ) speed = 4.0f;
    
    this->_speed = speed;
    
    if ( _flags.prepared ) {
        _audio_renderer->setSpeed(speed);
    }
}

void AudioPlayer::setDefaultOutputDevice(OH_AudioDevice_Type device_type) {
    std::lock_guard<std::mutex> lock(mtx);
    this->_device_type = device_type;
    
    if ( _flags.prepared ) {
        _audio_renderer->setDefaultOutputDevice(device_type);
    }
}

void AudioPlayer::setEventCallback(EventMessageQueue::EventCallback callback) {
    _event_msg_queue->setEventCallback(callback);
}

int64_t AudioPlayer::getDurationPlayed() const  {
    auto duration = _duration_played.load(std::__n1::memory_order_relaxed);
    return duration > 0 ? av_rescale_q(duration, (AVRational){ 1, _output_sample_rate }, (AVRational){ 1, 1000 }) : 0;
}

void AudioPlayer::onPrepare() {
    if ( _flags.prepared || _flags.has_error || _flags.released ) {
        return;
    }
    
    // init audio renderer
    _audio_renderer = new AudioRenderer();
    OH_AudioStream_Result render_ret = _audio_renderer->init(FFAV::Conversion::toOHFormat(_output_sample_format), _output_sample_rate, _output_channels, _options.stream_usage);
    if ( render_ret != AUDIOSTREAM_SUCCESS ) {
        onRenderError(render_ret);
        return;
    }
    
    if ( _volume != 1 ) _audio_renderer->setVolume(_volume);
    if ( _speed != 1 ) _audio_renderer->setSpeed(_speed);
    if ( _device_type != OH_AudioDevice_Type::AUDIO_DEVICE_TYPE_DEFAULT ) _audio_renderer->setDefaultOutputDevice(_device_type);
    
    // set callbacks
    _audio_renderer->setWriteDataCallback(std::bind(&AudioPlayer::onRendererWriteDataCallback, this, std::placeholders::_1, std::placeholders::_2));
    _audio_renderer->setInterruptEventCallback(std::bind(&AudioPlayer::onRendererInterruptEventCallback, this, std::placeholders::_1, std::placeholders::_2));
    _audio_renderer->setErrorCallback(std::bind(&AudioPlayer::onRendererErrorCallback, this, std::placeholders::_1));
    _audio_renderer->setOutputDeviceChangeCallback(std::bind(&AudioPlayer::onOutputDeviceChangeCallback, this, std::placeholders::_1));

    // init audio item & prepare
    AudioItem::Options item_options;
    item_options.http_options = _options.http_options;
    item_options.output_sample_rate = _output_sample_rate;
    item_options.output_sample_format = _output_sample_format;
    item_options.output_channels = _output_channels;
    if ( _options.start_time_position_ms > 0 ) {
        item_options.start_time_pos = av_rescale_q(_options.start_time_position_ms, (AVRational){ 1, 1000 }, AV_TIME_BASE_Q);
    }
    
    _audio_item = onCreateAudioItem(_url, item_options);
    _audio_item->setStreamReadyCallback([&](int64_t duration, AVRational time_base) {
        _duration_ms = av_rescale_q(duration, time_base, (AVRational){ 1, 1000 });
        onEvent(std::make_shared<DurationChangeEventMessage>(_duration_ms)); 
    });
    _audio_item->setBufferedTimeChangeCallback([&](int64_t buffered_time, AVRational time_base) {
        onEvent(std::make_shared<PlayableDurationChangeEventMessage>(av_rescale_q(buffered_time, time_base, (AVRational){ 1, 1000 })));
    });
    _audio_item->setErrorCallback([&](int ff_err) {
        onFFmpegError(ff_err);
    });
    // prepare
    _audio_item->prepare();
    
    _flags.prepared = true;
}

OH_AudioData_Callback_Result AudioPlayer::onRendererWriteDataCallback(void* write_buffer, int write_buffer_size_in_bytes) {
    std::unique_lock<std::mutex> lock(mtx);
    
    int capacity = write_buffer_size_in_bytes / _output_bytes_per_sample / _output_channels;
    int64_t pts = 0;
    bool eof = false;
    int ret = _flags.prepared ? _audio_item->tryTranscode(&write_buffer, capacity, &pts, &eof) : 0;
    
    int samples_read = ret > 0 ? ret : 0;
    if ( samples_read < capacity ) {
        int bytes_read = samples_read * _output_channels * _output_bytes_per_sample;
        memset(static_cast<uint8_t*>(write_buffer) + bytes_read, 0, write_buffer_size_in_bytes - bytes_read);
    }
    
    // playback ended
    if ( samples_read == 0 && eof ) {
        _flags.is_playback_ended = true;
        onEvent(std::make_shared<CurrentTimeEventMessage>(_duration_ms));
        onPause(PlayWhenReadyChangeReason::PLAYBACK_ENDED);
    }
    // error
    else if ( ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF ) {
        onFFmpegError(ret);
    }
    else if ( samples_read > 0 ) {
        auto pts_ms = av_rescale_q(pts, (AVRational){ 1, _output_sample_rate }, (AVRational){ 1, 1000 });
        auto cur_ms = std::min(pts_ms, _duration_ms);
        _duration_played.fetch_add(samples_read);
        onEvent(std::make_shared<CurrentTimeEventMessage>(cur_ms));
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
                if ( _play_when_ready_change_reason == PlayWhenReadyChangeReason::AUDIO_INTERRUPT_PAUSE ) {
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
    if ( _flags.has_error || _flags.released ) {
        return;
    }
    
    _flags.has_error = true;
    
    if ( _flags.is_renderer_running ) {
        _audio_renderer->stop();
        _flags.is_renderer_running = false;
    }
    onEvent(std::make_shared<ErrorEventMessage>(error));
}

void AudioPlayer::onPlay(PlayWhenReadyChangeReason reason) {
    if ( _flags.has_error || _flags.released ) {
        return;
    }

    _play_when_ready_change_reason = reason;
    
    if ( !_flags.play_when_ready ) {
        _flags.play_when_ready = true;
        
        if ( !_flags.prepared ) {
            onPrepare();
        }
        
        if ( _flags.prepared ) {
            startRenderer();
        }
    }
    
    onEvent(std::make_shared<PlayWhenReadyChangeEventMessage>(true, reason));
}

void AudioPlayer::onPause(PlayWhenReadyChangeReason reason, bool should_invoke_pause) {
    
    if ( _flags.has_error || _flags.released ) {
        return;
    }
    
    _play_when_ready_change_reason = reason;
    
    if ( _flags.play_when_ready ) {
        _flags.play_when_ready = false;
        
        if ( _flags.is_renderer_running ) {
            // 当被系统强制中断时 render 没有必要执行 pause, 外部可以传递该参数决定是否需要调用render的暂停;
            if ( should_invoke_pause ) {
              OH_AudioStream_Result render_ret = _audio_renderer->pause();
                if ( render_ret != AUDIOSTREAM_SUCCESS ) {
                    onRenderError(render_ret);
                    return;
                }
            }
            
            _flags.is_renderer_running = false;
        }
    }
    
    onEvent(std::make_shared<PlayWhenReadyChangeEventMessage>(false, reason));
}

void AudioPlayer::onEvent(std::shared_ptr<EventMessage> msg) {
    _event_msg_queue->push(msg);
}

void AudioPlayer::startRenderer() {
    OH_AudioStream_Result render_ret = _audio_renderer->play();
    if ( render_ret != AUDIOSTREAM_SUCCESS ) {
        onRenderError(render_ret);
        return;
    }
    _flags.is_renderer_running = true;
}

AudioItem* AudioPlayer::onCreateAudioItem(const std::string& url, const AudioItem::Options& options) {
    return new AudioItem(url, options);
}

AudioItem *AudioPlayer::getAudioItem() {
    return _audio_item;
}

}