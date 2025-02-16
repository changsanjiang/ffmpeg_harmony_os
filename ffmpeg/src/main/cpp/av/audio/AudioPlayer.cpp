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

#include "AudioPlayer.h"
#include "av/core/AudioUtils.h"
#include <cstdint>
#include <sstream>
#include <algorithm> 

//#define DEBUG

#include "extension/client_print.h"

namespace FFAV {

const AVSampleFormat OUTPUT_SAMPLE_FORMAT = AV_SAMPLE_FMT_S16;
const OH_AudioStream_SampleFormat OUTPUT_RENDER_SAMPLE_FORMAT = AUDIOSTREAM_SAMPLE_S16LE;

const char* FILTER_BUFFER_SRC_NAME = "0:a";
const char* FILTER_BUFFER_SINK_NAME = "outa";

AudioPlayer::AudioPlayer(const std::string& url): url(url) {
    
}

AudioPlayer::~AudioPlayer() {
    std::unique_lock<std::mutex> lock(mtx);
    onRelease(lock);
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
    if ( !flags.init_successful || flags.has_error || flags.release_invoked ) {
        return;
    }
    if ( time_pos_ms < 0 ) time_pos_ms = 0;
    else if ( time_pos_ms > duration_ms ) time_pos_ms = duration_ms;
    flags.wants_seek = true;
    seek_time_ms = time_pos_ms;
    audio_reader->interrupt();
    lock.unlock();
    cv.notify_all();
}

void AudioPlayer::onRelease(std::unique_lock<std::mutex>& lock) {
    if ( flags.release_invoked ) {
        return;
    }
    
#ifdef DEBUG
    client_print_message3("AAAA: release before");
#endif
    
    flags.release_invoked = true;
    lock.unlock();
    cv.notify_all();

    audio_reader->interrupt();
    
    if ( init_thread && init_thread->joinable() ) {
        init_thread->join();
        init_thread.reset();
        init_thread = nullptr;
    }
    
    if ( read_thread && read_thread->joinable() ) {
        read_thread->join();
        read_thread.reset();
        read_thread = nullptr;
    }
    
    if ( dec_thread && dec_thread->joinable() ) {
        dec_thread->join();
        dec_thread.reset();
        dec_thread = nullptr;
    }
    
    if ( event_msg_thread && event_msg_thread->joinable() ) {
        event_msg_thread->join();
        event_msg_thread.reset();
        event_msg_thread = nullptr;
    }
    
    if ( audio_renderer ) {
        audio_renderer->stop();
        delete audio_renderer;
    }
    
    if ( pkt_queue ) {
        delete pkt_queue;
    }
    
    if ( audio_fifo ) {
        delete audio_fifo;
    }
    
    if ( filter_graph ) {
        delete filter_graph;
    }
    
    if ( audio_decoder ) {
        delete audio_decoder;
    }
    
    if ( audio_reader ) {
        delete audio_reader;
    }
    
    if ( !event_msg_queue.empty() ) {
        do {
            EventMessage* msg = event_msg_queue.front();
            event_msg_queue.pop();
            delete msg;
        } while (!event_msg_queue.empty());
    }
    
#ifdef DEBUG
    client_print_message3("AAAA: release after");
#endif
}

//int64_t AudioPlayer::getDuration() {
//    return duration_ms.load();
//}
//
//int64_t AudioPlayer::getCurrentTime() {
//    return last_render_pts_ms.load();
//}
//
//int64_t AudioPlayer::getPlayableDuration() {
//    return last_pkt_pts_or_end_time_ms.load();
//}
//
//std::shared_ptr<Error> AudioPlayer::getError() {
//    std::lock_guard<std::mutex> lock(mtx);
//    return cur_error;
//}

void AudioPlayer::setVolume(float volume) {
    if ( volume < 0.0f ) volume = 0.0f;
    else if ( volume > 1.0f ) volume = 1.0f;
    
    std::lock_guard<std::mutex> lock(mtx);
    if ( flags.has_error || flags.release_invoked ) {
        return;
    }
    
    this->volume = volume;
    if ( flags.init_successful ) {
        audio_renderer->setVolume(volume);
    }
}

void AudioPlayer::setSpeed(float speed) {
    if ( speed < 0.25f ) speed = 0.25f;
    else if ( speed > 4.0f ) speed = 4.0f;
    
    std::lock_guard<std::mutex> lock(mtx);
    if ( flags.has_error || flags.release_invoked ) {
        return;
    }
    
    this->speed = speed;
    if ( flags.init_successful ) {
        audio_renderer->setSpeed(speed);
    }
}

void AudioPlayer::setEventCallback(AudioPlayer::EventCallback callback) {
    std::unique_lock<std::mutex> lock(mtx);
    if ( flags.has_error || flags.release_invoked ) {
        return;
    }
    
    event_callback = callback;
    
    if ( event_callback ) {
        if ( !event_msg_thread ) {
            event_msg_thread = std::make_unique<std::thread>(&AudioPlayer::EventThread, this);
        }
        return;
    }
    
    if ( event_msg_thread && event_msg_thread->joinable() ) {
        event_msg_thread->detach();
        event_msg_thread.reset();
        event_msg_thread = nullptr;
    }
    
    lock.unlock();
    cv.notify_all();
}

void AudioPlayer::InitThread() {
    std::unique_lock<std::mutex> lock(mtx);
    if ( flags.has_error || flags.release_invoked ) {
        return;
    }
    
    int ff_ret = 0;
    OH_AudioStream_Result render_ret = AUDIOSTREAM_SUCCESS;

    out_sample_fmt = OUTPUT_SAMPLE_FORMAT;
    out_render_sample_fmt = OUTPUT_RENDER_SAMPLE_FORMAT;
    out_bytes_per_sample = av_get_bytes_per_sample(out_sample_fmt);
    
    AVStream* audio_stream;
    
    AVBufferSrcParameters *buf_src_params = nullptr;
    AVChannelLayout out_ch_layout;
    int64_t nb_render_frame_samples;

    lock.unlock();
    // init reader
    ff_ret = audio_reader->open(url);
    if ( ff_ret < 0 ) {
        goto exit_thread;
    }
    
    lock.lock();
    if ( flags.has_error || flags.release_invoked ) {
        goto exit_thread;
    }
    
    // find audio stream
    audio_stream = audio_reader->getBestStream(AVMEDIA_TYPE_AUDIO);
    if ( audio_stream == nullptr ) {
        ff_ret = AVERROR_STREAM_NOT_FOUND;
        goto exit_thread;
    }
    
    
    audio_stream_index = audio_stream->index;
    time_base = audio_stream->time_base;
    duration_ms = av_rescale_q(audio_stream->duration, time_base, (AVRational){ 1, 1000 });
    
    // init decoder
    ff_ret = initDecoder(audio_stream->codecpar);
    if ( ff_ret < 0 ) {
        goto exit_thread;
    }
    
    buf_src_params = audio_decoder->createBufferSrcParameters(audio_stream->time_base);
    out_sample_rate = buf_src_params->sample_rate;
    out_ch_layout = buf_src_params->ch_layout;
    out_nb_channels = out_ch_layout.nb_channels;
    char ch_layout_desc[64];
    av_channel_layout_describe(&out_ch_layout, ch_layout_desc, sizeof(ch_layout_desc)); // get channel layout desc
    out_ch_layout_desc = ch_layout_desc;
    
    // init filter graph
    ff_ret = initFilterGraph(
        FILTER_BUFFER_SRC_NAME, 
        FILTER_BUFFER_SINK_NAME, 
        buf_src_params,
        out_sample_fmt,
        out_sample_rate,
        out_ch_layout_desc
    );
    if ( ff_ret < 0 ) {
        goto exit_thread;
    }    
    
    // init renderer
    render_ret = initRenderer(out_render_sample_fmt, out_sample_rate, out_nb_channels);
    if ( render_ret != AUDIOSTREAM_SUCCESS ) {
        goto exit_thread;
    }
    
    nb_render_frame_samples = audio_renderer->getFrameSize();
    
    // init audio fifo
    ff_ret = initAudioFifo(out_sample_fmt, out_nb_channels, nb_render_frame_samples);
    if ( ff_ret < 0 ) {
        goto exit_thread;
    }
    
    // init packet queue
    ff_ret = initPacketQueue();
    if ( ff_ret < 0 ) {
        goto exit_thread;
    }
    
    frame_threshold = std::max(av_rescale_q(1000, (AVRational){ 1, 1000 }, (AVRational) { 1, out_sample_rate }), nb_render_frame_samples * 5);
    
//    flags.wants_seek = true;
//    seek_time_ms = 5000;
    
    // start read & dec threads
    read_thread = std::make_unique<std::thread>(&AudioPlayer::ReadThread, this);
    dec_thread =  std::make_unique<std::thread>(&AudioPlayer::DecThread, this);
    
    onEvent(new DurationChangeEventMessage(duration_ms));
    
exit_thread:
    if ( buf_src_params != nullptr ) {
        av_free(buf_src_params);
    }
    
    flags.init_successful = (ff_ret == 0 && render_ret == AUDIOSTREAM_SUCCESS);

    if ( flags.init_successful ) {
        lock.unlock();
        cv.notify_all();
    }
    else {
        ff_ret < 0 ? onFFmpegError(ff_ret, lock) : onRenderError(render_ret, lock);
    }
    
#ifdef DEBUG
    client_print_message3("AAAA: init thread exit");
#endif
}

void AudioPlayer::ReadThread() {
    int ret = 0;
    AVPacket* pkt = av_packet_alloc();
    bool should_notify;
    bool should_seek;
    bool should_restart;
    bool should_exit;
    bool error_occurred;
    
    do {
restart:        
        ret = 0;
        should_notify = false;
        should_seek = false;
        should_restart = false;
        should_exit = false;
        error_occurred = false;
        
        // wait conditions
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [&] { 
                if ( flags.has_error || flags.release_invoked ) {
                    return true;
                }
                
                if ( flags.wants_seek ) {
                    return true;
                }
                
                return !flags.is_read_eof && pkt_queue->getSize() < pkt_size_threshold;
            });
            
            if ( flags.has_error || flags.release_invoked ) {
                should_exit = true;
            }
            else if ( flags.wants_seek ) {
                flags.wants_seek = false;
                flags.is_seeking = true;
                should_seek = true;
                should_notify = true;
            }
            
            if ( !should_exit ) {
                if ( init_thread && init_thread->joinable() ) {
#ifdef DEBUG
                    client_print_message3("AAAA: clear init thread =%p", init_thread.get());
#endif
                    // clear init thread
                    init_thread->detach();
                    init_thread.reset();
                    init_thread = nullptr;
                }
            }
        }
        
        if ( should_exit ) {
            goto exit_thread;
        }
        
        if ( should_notify ) {
            cv.notify_all();
        }
        
        // handle seek
        if ( should_seek ) {
#ifdef DEBUG
            client_print_message3("AAAA: seek to %ld", seek_time_ms);
#endif
            int64_t seek_time = av_rescale_q(seek_time_ms, (AVRational){ 1, 1000 }, AV_TIME_BASE_Q);
            ret = audio_reader->seek(seek_time, -1);
            
            std::lock_guard<std::mutex> lock(mtx);
            if ( flags.has_error || flags.release_invoked ) {
                should_exit = true;
            }
            else if ( ret < 0 ) {
                if ( ret == AVERROR_EOF ) {
                    // nothing
                }
                else if ( ret == AVERROR_EXIT ) {
                    // interrupted
                    // restart
                    should_restart = true;
                }
                else {
                    // error
                    error_occurred = true;
                }
            }
            else {
                // clear buffers
                audio_decoder->flush();
                audio_fifo->clear();
                pkt_queue->clear();
                if ( !flags.is_playing ) {
                    audio_renderer->flush();    
                }
                
                ret = resetFilterGraph();
                if ( ret < 0 ) {
                    error_occurred = true;
                }
                else {
                    // seek finished
                    flags.is_seeking = false;
                    flags.is_read_eof = false;
                    flags.is_dec_eof = false;
                    flags.is_render_eof = false;
                    should_notify = true;
                }
            }
        }
        
        if ( should_exit || error_occurred ) {
            goto exit_thread;
        }
        
        if ( should_restart ) {
            goto restart;
        }
         
        // read packet
        ret = audio_reader->readPacket(pkt);
        {
            std::lock_guard<std::mutex> lock(mtx);
            if ( flags.has_error || flags.release_invoked ) {
                should_exit = true;
            }
            else if ( ret < 0 ) {
                if ( ret == AVERROR_EXIT ) {
                    // interrupted
                    // nothing
                }
                else if ( ret == AVERROR_EOF ) {
#ifdef DEBUG
                    client_print_message3("AAAA: read eof");
#endif
                    
                    // read eof
                    flags.is_read_eof = true;
                    last_pkt_pts_or_end_time_ms = duration_ms;
                    onEvent(new PlayableDurationChangeEventMessage(duration_ms));
                    should_notify = true;
                }
                else {
                    // error
                    error_occurred = true;
                }
            }
                // push pkt
            else if ( pkt->stream_index == audio_stream_index ) {
#ifdef DEBUG
                client_print_message3("AAAA: read pkt: pts=%ld", pkt->pts);
#endif
                pkt_queue->push(pkt);
                
                if ( pkt->pts != AV_NOPTS_VALUE ) {
                    int64_t pts_ms = av_rescale_q(pkt->pts, time_base, (AVRational){ 1, 1000 });
                    last_pkt_pts_or_end_time_ms = pts_ms;
                    onEvent(new PlayableDurationChangeEventMessage(pts_ms));
                }
                should_notify = true;
            }
            if ( ret == 0 ) av_packet_unref(pkt);
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
        std::unique_lock<std::mutex> lock(mtx);
        onFFmpegError(ret, lock);
    }
    
#ifdef DEBUG
    client_print_message3("AAAA: read thread exit");
#endif
}

void AudioPlayer::DecThread() {
    int ret = 0;
    AVPacket *pkt = av_packet_alloc();
    AVFrame *dec_frame = av_frame_alloc();
    AVFrame *filt_frame = av_frame_alloc();
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
                
                if ( flags.is_dec_eof || flags.is_seeking ) {
                    return false;
                }
                
                // frame buf 是否还可以继续填充
                if ( audio_fifo->getSize() < frame_threshold ) {
                    // 判断是否有可解码的 pkt 或 read eof;
                    return pkt_queue->getCount() > 0 || flags.is_read_eof;
                }
                
                return false;
            });
            
            if ( flags.has_error || flags.release_invoked ) {
                should_exit = true;
            }
            // dec pkt
            else if ( pkt_queue->pop(pkt) ) {
                ret = AudioUtils::transcode(
                    pkt,
                    audio_decoder, 
                    dec_frame, 
                    filter_graph, 
                    filt_frame, 
                    FILTER_BUFFER_SRC_NAME, 
                    FILTER_BUFFER_SINK_NAME, 
                    audio_fifo
                );
#ifdef DEBUG
                client_print_message3("AAAA: dec pkt: pts=%ld, fifo size: %ld", pkt->pts, audio_fifo->getSize());
#endif
                
                av_packet_unref(pkt);
                
                if ( ret < 0 && ret != AVERROR(EAGAIN) ) {
                    error_occurred = true;
                }
            }
            // read eof
            else if ( flags.is_read_eof ) {
#ifdef DEBUG
                client_print_message3("AAAA: dec eof, fifo size: %ld", audio_fifo->getSize());
#endif
                
                ret = AudioUtils::transcode(
                    nullptr,
                    audio_decoder, 
                    dec_frame, 
                    filter_graph, 
                    filt_frame, 
                    FILTER_BUFFER_SRC_NAME, 
                    FILTER_BUFFER_SINK_NAME, 
                    audio_fifo
                );
                
                // dec eof
                if ( ret == AVERROR_EOF ) {
                    flags.is_dec_eof = true;
                    should_notify = true;
                }
                else if ( ret < 0 ) {
                    error_occurred = true;
                }
            }
            
            onEvaluate();
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
    av_frame_free(&dec_frame);
    av_frame_free(&filt_frame);
    
    if ( error_occurred ) {
        std::unique_lock<std::mutex> lock(mtx);
        onFFmpegError(ret, lock);
    }
    
#ifdef DEBUG
    client_print_message3("AAAA: dec thread exit");
#endif
}

void AudioPlayer::EventThread() {
    EventMessage* event;
    EventCallback callback;
    do {
        event = nullptr;
        callback = nullptr;
        
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [&] { 
               return !event_msg_queue.empty() || flags.has_error || flags.release_invoked || !event_callback; 
            });
            
            if ( flags.has_error || flags.release_invoked ) {
                break; // exit thread
            }
            
            callback = event_callback;
            if ( !callback ) {
                break; // exit thread
            }
            
            event = event_msg_queue.front();
            event_msg_queue.pop();
        }
        
        if ( event ) {
            callback(event);
            delete event;
        }
    } while (true);
    
#ifdef DEBUG
    client_print_message3("AAAA: event thread exit");
#endif
}

int AudioPlayer::initDecoder(AVCodecParameters* codecpar) {
    int ff_ret = 0;
    audio_decoder = new MediaDecoder();
    ff_ret = audio_decoder->init(codecpar);
    if ( ff_ret < 0 ) {
        return ff_ret;
    }
    return 0;
}

int AudioPlayer::initFilterGraph(
    const char* buf_src_name,
    const char* buf_sink_name,
    AVBufferSrcParameters *buf_src_params, 
    AVSampleFormat out_sample_fmt,
    int out_sample_rate,
    const std::string& out_ch_layout_desc
) {
    int ff_ret = 0;

    // init filter graph
    filter_graph = new FilterGraph();
    ff_ret = filter_graph->init();
    if ( ff_ret < 0 ) {
        return ff_ret;
    }
    
    // cfg filter graph
    const AVSampleFormat out_sample_fmts[] = { out_sample_fmt, AV_SAMPLE_FMT_NONE };
    const int out_sample_rates[] = { out_sample_rate, -1 };
    
    std::stringstream filter_descr_ss;
    filter_descr_ss << "[" << buf_src_name << "]"
                    << "aformat=sample_fmts=" << av_get_sample_fmt_name(out_sample_fmt) << ":channel_layouts=" << out_ch_layout_desc
                    << "[" << buf_sink_name << "]";
    std::string filter_descr = filter_descr_ss.str();
    
    // add buffersrc [0:a]
    ff_ret = filter_graph->addBufferSourceFilter(buf_src_name, AVMEDIA_TYPE_AUDIO, buf_src_params);
    if ( ff_ret < 0 ) {
        return ff_ret;
    }
    
    // add buffersink [outa]
    ff_ret = filter_graph->addAudioBufferSinkFilter(buf_sink_name, out_sample_rates, out_sample_fmts, out_ch_layout_desc);
    if ( ff_ret < 0 ) {
        return ff_ret;
    }

    ff_ret = filter_graph->parse(filter_descr);
    if ( ff_ret < 0 ) {
        return ff_ret;
    }
    
    ff_ret = filter_graph->configure();
    if ( ff_ret < 0 ) {
        return ff_ret;
    } 
    return 0;
}

int AudioPlayer::initAudioFifo(
    AVSampleFormat sample_fmt, 
    int nb_channels, 
    int nb_samples
) {
    int ff_ret = 0;
    
    // init audio fifo buffer
    audio_fifo = new AudioFifo();
    ff_ret = audio_fifo->init(sample_fmt, nb_channels, nb_samples);
    if ( ff_ret < 0 ) {
        return ff_ret;
    }

    return 0;
}

int AudioPlayer::initPacketQueue() {
    pkt_queue = new PacketQueue();
    return 0;
}

OH_AudioStream_Result AudioPlayer::initRenderer(
    OH_AudioStream_SampleFormat sample_fmt, 
    int sample_rate,
    int nb_channels
) {
    OH_AudioStream_Result render_ret = AUDIOSTREAM_SUCCESS;
    // init audio renderer
    audio_renderer = new AudioRenderer();
    render_ret = audio_renderer->init(sample_fmt, sample_rate, nb_channels);
    if ( render_ret != AUDIOSTREAM_SUCCESS ) { 
        return render_ret;
    }
    
    if ( volume != 1.0f ) audio_renderer->setVolume(volume);
    if ( speed != 1.0f ) audio_renderer->setSpeed(speed);

    // set callbacks
    audio_renderer->setWriteDataCallback(std::bind(&AudioPlayer::onRendererWriteDataCallback, this, std::placeholders::_1, std::placeholders::_2));
    audio_renderer->setInterruptEventCallback(std::bind(&AudioPlayer::onRendererInterruptEventCallback, this, std::placeholders::_1, std::placeholders::_2));
    audio_renderer->setErrorCallback(std::bind(&AudioPlayer::onRendererErrorCallback, this, std::placeholders::_1));
    audio_renderer->setOutputDeviceChangeCallback(std::bind(&AudioPlayer::onOutputDeviceChangeCallback, this, std::placeholders::_1));
    return AUDIOSTREAM_SUCCESS;
}

int AudioPlayer::resetFilterGraph() {
    delete filter_graph;
    AVBufferSrcParameters *buf_src_params = audio_decoder->createBufferSrcParameters(time_base);
    int ret = initFilterGraph(
        FILTER_BUFFER_SRC_NAME, 
        FILTER_BUFFER_SINK_NAME, 
        buf_src_params,
        out_sample_fmt,
        out_sample_rate,
        out_ch_layout_desc
    );
    av_free(buf_src_params);
    return ret;
}

void AudioPlayer::onFFmpegError(int error, std::unique_lock<std::mutex>& lock) {
    client_print_message3("AAAAA: onFFmpegError(%d), %s", error, av_err2str(error));
    
    onError(Error::FFError(error), lock);
}

void AudioPlayer::onRenderError(OH_AudioStream_Result error, std::unique_lock<std::mutex>& lock) {
    client_print_message3("AAAAA: onRenderError(%d)", error);

    onError(Error::RenderError(error), lock);
}

void AudioPlayer::onError(std::shared_ptr<Error> error, std::unique_lock<std::mutex>& lock) {
    if ( flags.has_error || flags.release_invoked ) {
        return;
    }
    
    flags.has_error = true;
    cur_error = error;
    
    if ( flags.init_successful ) {
        audio_reader->interrupt();
    }
    lock.unlock();
    cv.notify_all();
    
    lock.lock();
    if ( flags.release_invoked ) {
        return;
    }
    
    if ( flags.is_playing ) {
        audio_renderer->stop();
        flags.is_playing = false;
    }
    onEvent(new ErrorEventMessage(error));
}

void AudioPlayer::onPrepare() {
    if ( flags.prepare_invoked || flags.has_error || flags.release_invoked ) {
        return;
    }
    
    flags.prepare_invoked = true;
    audio_reader = new MediaReader();
    init_thread = std::make_unique<std::thread>(&AudioPlayer::InitThread, this);
}

void AudioPlayer::onEvaluate() {
    if ( flags.is_playing || flags.is_render_eof || !flags.play_when_ready || flags.has_error || flags.release_invoked ) {
        return;
    }
    
    int64_t buf_duration_ms = av_rescale_q(audio_fifo->getSize(), (AVRational) { 1, out_sample_rate }, (AVRational) { 1, 1000 });
    bool should_play = buf_duration_ms >= 500 || flags.is_dec_eof;
    
    if ( should_play ) {
        OH_AudioStream_Result ret = audio_renderer->play();
        flags.is_playing = ret == AUDIOSTREAM_SUCCESS;
    }
}

void AudioPlayer::onRenderEnd() {
    onPause(PlayWhenReadyChangeReason::RENDER_END);
 
#ifdef DEBUG
    client_print_message("AAAA: playback ended");
#endif
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
        else if ( flags.init_successful ) {
            onEvaluate();
        }
    }
    
    onEvent(new PlayWhenReadyChangeEventMessage(true, reason));
}

void AudioPlayer::onPause(PlayWhenReadyChangeReason reason, bool should_invoke_pause) {
    
    if ( flags.has_error || flags.release_invoked ) {
        return;
    }
    
    play_when_ready_change_reason = reason;
    
    if ( flags.play_when_ready ) {
        flags.play_when_ready = false;
        
        if ( flags.is_playing ) {
            // 当被系统强制中断时 render 没有必要执行 pause, 外部可以传递该参数决定是否需要调用render的暂停;
            if ( should_invoke_pause ) {
                audio_renderer->pause();
            }
            flags.is_playing = false;
        }
    }
    
    onEvent(new PlayWhenReadyChangeEventMessage(false, reason));
}

void AudioPlayer::onEvent(EventMessage* msg) {
    if ( flags.release_invoked || !event_callback ) {
        delete msg;
        return;
    }
    
    event_msg_queue.push(msg);
    cv.notify_all();
}

OH_AudioData_Callback_Result AudioPlayer::onRendererWriteDataCallback(void* audio_buffer, int audio_buffer_size_in_bytes) {
    std::unique_lock<std::mutex> lock(mtx);
    if ( flags.release_invoked || flags.has_error ) {
        return AUDIO_DATA_CALLBACK_RESULT_INVALID;
    }
    
    if ( flags.is_render_eof ) {
        last_render_pts_ms = duration_ms;
        onEvent(new CurrentTimeEventMessage(last_render_pts_ms));
        onRenderEnd();
        return AUDIO_DATA_CALLBACK_RESULT_INVALID;
    }
    
    int audio_buffer_samples = audio_buffer_size_in_bytes / out_nb_channels / out_bytes_per_sample;
    if ( audio_fifo->getSize() >= audio_buffer_samples || flags.is_dec_eof ) {
        int64_t pts;
        int ff_ret = audio_fifo->read(&audio_buffer, audio_buffer_samples, &pts);
        if ( ff_ret < 0 ) {
            onFFmpegError(ff_ret, lock);
            return AUDIO_DATA_CALLBACK_RESULT_INVALID;
        }
        int64_t pts_ms = av_rescale_q(pts, (AVRational){ 1, out_sample_rate }, (AVRational){ 1, 1000 });
        last_render_pts_ms = pts_ms;
        onEvent(new CurrentTimeEventMessage(pts_ms));
        
        if ( flags.is_dec_eof ) {
            int read_samples = ff_ret;
            int read_bytes = read_samples * out_nb_channels * out_bytes_per_sample;
            if ( read_bytes < audio_buffer_size_in_bytes ) {
                memset(static_cast<uint8_t*>(audio_buffer) + read_bytes, 0, audio_buffer_size_in_bytes - read_bytes);
                flags.is_render_eof = true;
            }
        }
        
        cv.notify_all(); // notify read
        return AUDIO_DATA_CALLBACK_RESULT_VALID;
    }
    return AUDIO_DATA_CALLBACK_RESULT_INVALID;
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
    onRenderError(error, lock);
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

}