//
// Created on 2025/2/25.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "AudioPlayer.h"
#include "extension/client_print.h"
#include <cerrno>
#include <stdint.h>

namespace FFAV {

static const AVSampleFormat OUTPUT_SAMPLE_FORMAT = AV_SAMPLE_FMT_S16;
static const OH_AudioStream_SampleFormat OUTPUT_RENDER_SAMPLE_FORMAT = AUDIOSTREAM_SAMPLE_S16LE;

AudioPlayer::AudioPlayer(const std::string& url, int64_t start_time_position_ms): url(url), start_time_position_ms(start_time_position_ms) {

}

AudioPlayer::~AudioPlayer() {
#ifdef DEBUG
    client_print_message3("AAAA: AudioPlayer::~AudioPlayer before");
#endif
    std::unique_lock<std::mutex> lock(mtx);
    flags.release_invoked = true;
    lock.unlock();
    cv.notify_all();
    
    event_msg_queue->stop();
    delete event_msg_queue;
    
    if ( dec_thread && dec_thread->joinable() ) {
        dec_thread->join();
    }
    
    if ( audio_renderer ) {
        audio_renderer->stop();
        delete audio_renderer;
    }

    if ( audio_reader ) {
        audio_reader->stop();
        delete audio_reader;
    }
    
    if ( audio_decoder ) {
        delete audio_decoder;
    }
    
    if ( pkt_queue ) {
        delete pkt_queue;
    }
    
    if ( audio_fifo ) {
        delete audio_fifo;
    }
    
#ifdef DEBUG
    client_print_message3("AAAA: AudioPlayer::~AudioPlayer after");
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

void AudioPlayer::playImmediately() {
    std::lock_guard<std::mutex> lock(mtx);
    flags.should_play_immediate = true;
    onPlay(PlayWhenReadyChangeReason::USER_REQUEST);
}

void AudioPlayer::pause() {
    std::lock_guard<std::mutex> lock(mtx);
    onPause(PlayWhenReadyChangeReason::USER_REQUEST);
}

void AudioPlayer::seek(int64_t time_pos_ms) {
    std::unique_lock<std::mutex> lock(mtx);
    if ( !flags.init_successful || flags.has_error || flags.release_invoked ) {
        return;
    }
    
    if ( flags.should_recreate_reader ) {
        if ( recreate_reader_scheduler && !recreate_reader_scheduler->tryCancel() ) {
            return;
        }
        flags.should_recreate_reader = false;
        flush();
        recreateReader(time_pos_ms);
        return;
    }
    
    audio_reader->seek(time_pos_ms);
}

void AudioPlayer::setVolume(float volume) {
    std::lock_guard<std::mutex> lock(mtx);
    if ( flags.has_error || flags.release_invoked ) {
        return;
    }
    
    if ( volume < 0.0f ) volume = 0.0f;
    else if ( volume > 1.0f ) volume = 1.0f;
    
    this->volume = volume;
    if ( audio_renderer ) {
        audio_renderer->setVolume(volume);
    }
}

void AudioPlayer::setSpeed(float speed) {
    std::lock_guard<std::mutex> lock(mtx);
    if ( flags.has_error || flags.release_invoked ) {
        return;
    }
    
    if ( speed < 0.25f ) speed = 0.25f;
    else if ( speed > 4.0f ) speed = 4.0f;
    
    this->speed = speed;
    if ( audio_renderer ) {
        audio_renderer->setSpeed(speed);
    }
}

void AudioPlayer::setEventCallback(EventMessageQueue::EventCallback callback) {
    event_msg_queue->setEventCallback(callback);
}

void AudioPlayer::onPrepare() {
    if ( flags.prepare_invoked || flags.has_error || flags.release_invoked ) {
        return;
    }
    
    flags.prepare_invoked = true;
    recreateReader(start_time_position_ms);
}

void AudioPlayer::recreateReader(int64_t start_time_position_ms) {
    client_print_message3("AAAAA: AudioPlayer::recreateReader(%lld)", start_time_position_ms);
    if ( audio_reader ) {
        delete audio_reader;
    }
    
    audio_reader = new AudioReader(url, start_time_position_ms);
    audio_reader->setReadyToReadPacketCallback(std::bind(&AudioPlayer::onReaderReadyToReadCallback, this, std::placeholders::_1, std::placeholders::_2));
    audio_reader->setReadPacketCallback(std::bind(&AudioPlayer::onReaderReadPacketCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    audio_reader->setErrorCallback(std::bind(&AudioPlayer::onReaderErrorCallback, this, std::placeholders::_1, std::placeholders::_2));
    audio_reader->prepare();
}

void AudioPlayer::onReaderReadyToReadCallback(AudioReader* reader, AVStream* stream) {
    std::unique_lock<std::mutex> lock(mtx);
    if ( flags.has_error || flags.release_invoked ) {
        return;
    } 
    
    // 网络断开重连时新的 reader 回调会重新进这里, 以下初始化代码无需再次执行, 直接return即可;
    if ( flags.init_successful ) {
        reader->start();
        return;
    }
    
    // init audio decoder
    audio_decoder = new AudioDecoder();
    int ff_ret = audio_decoder->init(stream->codecpar, stream->time_base, OUTPUT_SAMPLE_FORMAT);
    if ( ff_ret < 0 ) {
        lock.unlock();
        onFFmpegError(ff_ret);
        return;
    }
    
    // init audio fifo
    audio_fifo = new AudioFifo();
    ff_ret = audio_fifo->init(OUTPUT_SAMPLE_FORMAT, audio_decoder->getOutputChannels(), 1);
    if ( ff_ret < 0 ) {
        lock.unlock();
        onFFmpegError(ff_ret);
        return;
    }
    
    // init pkt queue
    pkt_queue = new PacketQueue();
    
    // init audio renderer
    audio_renderer = new AudioRenderer();
    OH_AudioStream_Result render_ret = audio_renderer->init(OUTPUT_RENDER_SAMPLE_FORMAT, audio_decoder->getOutputSampleRate(), audio_decoder->getOutputChannels());
    if ( render_ret != AUDIOSTREAM_SUCCESS ) {
        lock.unlock();
        onRenderError(render_ret);
        return;
    }
    
    if ( volume != 1 ) audio_renderer->setVolume(volume);
    if ( speed != 1 ) audio_renderer->setSpeed(speed);
    
    // set callbacks
    audio_renderer->setWriteDataCallback(std::bind(&AudioPlayer::onRendererWriteDataCallback, this, std::placeholders::_1, std::placeholders::_2));
    audio_renderer->setInterruptEventCallback(std::bind(&AudioPlayer::onRendererInterruptEventCallback, this, std::placeholders::_1, std::placeholders::_2));
    audio_renderer->setErrorCallback(std::bind(&AudioPlayer::onRendererErrorCallback, this, std::placeholders::_1));
    audio_renderer->setOutputDeviceChangeCallback(std::bind(&AudioPlayer::onOutputDeviceChangeCallback, this, std::placeholders::_1));

    // init vars
    audio_stream_time_base = stream->time_base;
    output_time_base = (AVRational){ 1, audio_decoder->getOutputSampleRate() };
    output_nb_channels = audio_decoder->getOutputChannels();
    output_nb_bytes_per_sample = av_get_bytes_per_sample(audio_decoder->getOutputSampleFormat());
    
    render_frame_size = audio_renderer->getFrameSize();
    maximum_frame_threshold = av_rescale_q(5000, (AVRational){ 1, 1000 }, output_time_base);
    minimum_frame_threshold = av_rescale_q(3000, (AVRational){ 1, 1000 }, output_time_base);
    duration_ms = av_rescale_q(stream->duration, audio_stream_time_base, (AVRational){ 1, 1000 });
        
    // start dec thread
    dec_thread =  std::make_unique<std::thread>(&AudioPlayer::DecThread, this);
    
    // start read pkt
    reader->start();
    
    // init successful
    flags.init_successful = true;
    if ( flags.play_when_ready ) {
        startRenderer();
    }
    
    // notify events
    onEvent(std::make_shared<DurationChangeEventMessage>(duration_ms));
}

void AudioPlayer::onReaderReadPacketCallback(AudioReader* reader, AVPacket* pkt, bool should_flush) {
    std::unique_lock<std::mutex> lock(mtx);
    if ( flags.has_error || flags.release_invoked ) {
        return;
    }
    
    if ( should_flush || flags.should_flush_pkt ) {
        flush();
    }
    
    if ( pkt ) {
        pkt_queue->push(pkt);
        
        if ( pkt->pts != AV_NOPTS_VALUE ) {
            int64_t pts_ms = av_rescale_q(pkt->pts, audio_stream_time_base, (AVRational){ 1, 1000 });
            playable_duration_ms = pts_ms;
            onEvent(std::make_shared<PlayableDurationChangeEventMessage>(playable_duration_ms));
        }
    }
    else {
        flags.is_read_eof = true;
        playable_duration_ms = duration_ms;
        onEvent(std::make_shared<PlayableDurationChangeEventMessage>(playable_duration_ms));
    }
    
    reader->setPacketBufferFull(pkt_queue->getSize() >= pkt_size_threshold);
    lock.unlock();
    cv.notify_all();
}

void AudioPlayer::onReaderErrorCallback(AudioReader* reader, int ff_err) {
    client_print_message3("AAAAA: onReaderErrorCallback(%d), %s", ff_err, av_err2str(ff_err));
   
    if ( ff_err == AVERROR(EIO) ||
         ff_err == AVERROR(ENETDOWN) || 
         ff_err == AVERROR(ENETUNREACH) ||
         ff_err == AVERROR(ENETRESET) ||
         ff_err == AVERROR(ECONNABORTED) ||
         ff_err == AVERROR(ECONNRESET) || 
         ff_err == AVERROR(ETIMEDOUT) || 
         ff_err == AVERROR(EHOSTUNREACH)
        ) {
        std::unique_lock<std::mutex> lock(mtx);
        flags.should_recreate_reader = true;
        recreate_reader_scheduler = TaskScheduler::scheduleTask([&] {
            std::unique_lock<std::mutex> lock(mtx);
            if ( flags.has_error || flags.release_invoked ) {
                return;
            }
            
            // 如果之前未完成初始化, 则直接重新创建 reader 即可;
            // 如果已完成初始化, 则需要考虑读取的开始位置;
            // - 等待期间用户可能调用seek, 需要从seek的位置开始播放(模糊位置)
            // - 如果未执行seek操作, 则需要从当前位置开始播放(精确位置), 需要在解码时对齐数据
            if ( !flags.init_successful ) {
                recreateReader(start_time_position_ms);
                return;
            }
            
            // 如果等待期间用户执行了 seek 操作, 则 recreate_reader_task 会被取消;
            // 这里仅需考虑从当前位置恢复读取即可;
            // 从当前fifo的位置进行恢复, 新的解码数据需要对齐fifo后才能添加到fifo中;

            // reset flags 
            flags.should_align_pts = true; // 解码数据需要对齐到当前 fifo 的尾部;
            flags.should_flush_pkt = true; // reader 重新创建完毕后, 需要 flush 掉 pkt 的缓存;
            flags.should_recreate_reader = false;
            
            int64_t start_pts = audio_fifo->getEndPts();
            if ( start_pts == AV_NOPTS_VALUE ) { 
                start_pts = 0; 
            }
            int64_t pts_ms = av_rescale_q(start_pts, output_time_base, (AVRational){ 1, 1000 });
            recreateReader(pts_ms);
            recreate_reader_scheduler = nullptr;
        }, 5);
        return;
    }
    
    onFFmpegError(ff_err);
}

void AudioPlayer::DecThread() {
    int ret = 0;
    AVPacket* pkt = av_packet_alloc();
    bool should_notify;
    bool should_exit;
    bool error_occurred;

    do {
        should_notify = false;
        should_exit = false;
        error_occurred = false;
        {
            // wait conditions
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [&] { 
                if ( flags.has_error || flags.release_invoked ) {
                    return true;
                }
                
                if ( flags.is_dec_eof ) {
                    return false;
                }
                
                // frame buf 是否还可以继续填充
                if ( audio_fifo->getNumberOfSamples() < maximum_frame_threshold ) {
                    // 判断是否有可解码的 pkt 或 read eof;
                    return pkt_queue->getCount() > 0 || flags.is_read_eof;
                }
                
                return false;
            });
            
            if ( flags.has_error || flags.release_invoked ) {
                should_exit = true;
            }
            // dec pkt
            else {
                AVPacket* next_pkt = nullptr;
                if ( pkt_queue->pop(pkt) ) {
                    next_pkt = pkt;
                }
                
                ret = audio_decoder->decode(next_pkt, [&](AVFrame* filt_frame) {
                    if ( !flags.should_flush_pkt && flags.should_align_pts ) {
                        int64_t aligned_pts = audio_fifo->getEndPts(); 
                        int64_t start_pts = filt_frame->pts;
                        if ( aligned_pts != AV_NOPTS_VALUE && aligned_pts != start_pts ) {
                            if ( start_pts > aligned_pts ) {
                                return AVERROR_BUG2;
                            }
                            
                            int64_t end_pts = start_pts + filt_frame->nb_samples;
                            if ( aligned_pts > end_pts ) {
                                return 0;
                            }
                            
                            // intersecting samples
                            int64_t nb_samples = end_pts - aligned_pts;
                            int pos_offset = (aligned_pts - start_pts) * output_nb_bytes_per_sample * output_nb_channels;
                            uint8_t* data = *filt_frame->data;
                            data += pos_offset;
                            audio_fifo->write((void **)&data, nb_samples, aligned_pts);
                            flags.should_align_pts = false;
                            return 0;
                        }
                        else {
                            flags.should_align_pts = false;
                        }
                    }
                    audio_fifo->write((void **)filt_frame->data, filt_frame->nb_samples, filt_frame->pts);
                    return 0;
                });
                
                if ( next_pkt ) {
                    av_packet_unref(next_pkt);
                    audio_reader->setPacketBufferFull(pkt_queue->getSize() >= pkt_size_threshold);
                }
                
                if ( ret == AVERROR_EOF ) {
                    flags.is_dec_eof = true;
                    should_notify = true; // notify renderer for dec eof
                }
                else if ( ret == AVERROR(EAGAIN) ) {
                    // nothing
                }
                else if ( ret < 0 ) {
                    error_occurred = true;
                }
                
                if ( !error_occurred ) {
                    if ( flags.should_reset_current_time && audio_fifo->getNumberOfSamples() > 0 ) {
                        flags.should_reset_current_time = false;
                        current_time_ms = av_rescale_q(audio_fifo->getNextPts(), output_time_base, (AVRational){ 1, 1000 });
                        onEvent(std::make_shared<CurrentTimeEventMessage>(current_time_ms));
                    }
                }
            }
        }
        
        if ( should_exit || error_occurred ) {
            goto exit_thread;
        }
        
        if ( should_notify ) {
            cv.notify_all();
        }
    } while (true);
    
exit_thread:
    av_packet_free(&pkt);
    
    if ( error_occurred ) {
        onFFmpegError(ret);
    }
    
#ifdef DEBUG
    client_print_message3("AAAA: dec thread exit");
#endif
}

OH_AudioData_Callback_Result AudioPlayer::onRendererWriteDataCallback(void* write_buffer, int write_buffer_size_in_bytes) {
    std::unique_lock<std::mutex> lock(mtx);
    if ( flags.release_invoked || flags.has_error ) {
        return AUDIO_DATA_CALLBACK_RESULT_INVALID;
    }
    
    // playback ended
    if ( flags.is_render_eof && !flags.is_playback_ended ) {
        flags.is_playback_ended = true;
        current_time_ms = duration_ms;
        onEvent(std::make_shared<CurrentTimeEventMessage>(current_time_ms));
        onPause(PlayWhenReadyChangeReason::PLAYBACK_ENDED);
#ifdef DEBUG
    client_print_message("AAAA: playback ended");
#endif
        return AUDIO_DATA_CALLBACK_RESULT_INVALID;
    }
    
    int nb_write_buffer_samples = write_buffer_size_in_bytes / output_nb_channels / output_nb_bytes_per_sample;
    int nb_fifo_samples = audio_fifo->getNumberOfSamples();
    
    // 缓冲不足
    bool is_fifo_underflow = nb_fifo_samples < nb_write_buffer_samples && !flags.is_dec_eof;
    // 是否应该播放到缓冲耗尽
    bool should_drain_fifo = flags.should_drain_fifo;
    // 是否需要尽快播放
    // 当调用 playImmediately 或解码 eof 时直接播放
    bool play_immediate = (flags.should_play_immediate && nb_fifo_samples >= render_frame_size) || flags.is_dec_eof;
    // 是否能够尽可能的流畅播放
    bool keep_up_likely = (nb_fifo_samples >= minimum_frame_threshold);
    
    // reset flags
    if ( is_fifo_underflow && flags.should_drain_fifo ) {
        flags.should_drain_fifo = false;
    }
    if ( flags.should_play_immediate ) {
        flags.should_play_immediate = false;
    }
    
    // 是否需要读取fifo写入播放
    // 
    bool should_read_fifo = !is_fifo_underflow && (should_drain_fifo || play_immediate || keep_up_likely);
    if ( should_read_fifo ) {
        if ( !should_drain_fifo ) {
            flags.should_drain_fifo = true;
        }    
        
        int64_t pts;
        int nb_read_samples = audio_fifo->read(&write_buffer, nb_write_buffer_samples, &pts);
        int64_t pts_ms = av_rescale_q(pts, output_time_base, (AVRational){ 1, 1000 });
        current_time_ms = pts_ms;
        onEvent(std::make_shared<CurrentTimeEventMessage>(pts_ms));
        
        if ( flags.is_dec_eof ) {
            int read_bytes = nb_read_samples * output_nb_channels * output_nb_bytes_per_sample;
            if ( read_bytes < write_buffer_size_in_bytes ) {
                memset(static_cast<uint8_t*>(write_buffer) + read_bytes, 0, write_buffer_size_in_bytes - read_bytes);
                flags.is_render_eof = true;
            }
        }
        
        lock.unlock();
        cv.notify_all(); // notify read
        return AUDIO_DATA_CALLBACK_RESULT_VALID;
    }
    
    // set mute
    memset(static_cast<uint8_t*>(write_buffer), 0, write_buffer_size_in_bytes);
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

void AudioPlayer::flush() {
    if ( flags.should_flush_pkt ) {
        // clear buffers
        pkt_queue->clear();
        audio_decoder->flush();
        
        // reset flags
        flags.should_flush_pkt = false;
    }
    else {
        // clear buffers
        pkt_queue->clear();
        audio_decoder->flush();
        audio_fifo->clear();
        if ( !flags.is_renderer_running ) audio_renderer->flush();
        
        // reset flags
        flags.is_read_eof = false;
        flags.is_dec_eof = false;
        flags.is_render_eof = false;
        flags.is_playback_ended = false;
        flags.should_reset_current_time = true;
        flags.should_drain_fifo = false;
    }
}

void AudioPlayer::onFFmpegError(int error) {
    client_print_message3("AAAAA: onFFmpegError(%d), %s", error, av_err2str(error));
    
    onError(Error::FFError(error));
}

void AudioPlayer::onRenderError(OH_AudioStream_Result error) {
    client_print_message3("AAAAA: onRenderError(%d)", error);

    onError(Error::RenderError(error));
}

void AudioPlayer::onError(std::shared_ptr<Error> error) {
    std::unique_lock<std::mutex> lock(mtx);
    if ( flags.has_error || flags.release_invoked ) {
        return;
    }
    
    flags.has_error = true;
    cur_error = error;
    
    if ( flags.is_renderer_running ) {
        audio_renderer->stop();
        flags.is_renderer_running = false;
    }
    onEvent(std::make_shared<ErrorEventMessage>(error));
    lock.unlock();
    cv.notify_all();
}

void AudioPlayer::onPlay(PlayWhenReadyChangeReason reason) {
    if ( flags.has_error || flags.release_invoked ) {
        return;
    }

    play_when_ready_change_reason = reason;
    
    if ( !flags.play_when_ready ) {
        flags.play_when_ready = true;
        
        if ( !flags.prepare_invoked ) {
            onPrepare();
        }
        
        if ( flags.init_successful ) {
            startRenderer();
        }
    }
    
    onEvent(std::make_shared<PlayWhenReadyChangeEventMessage>(true, reason));
}

void AudioPlayer::onPause(PlayWhenReadyChangeReason reason, bool should_invoke_pause) {
    
    if ( flags.has_error || flags.release_invoked ) {
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