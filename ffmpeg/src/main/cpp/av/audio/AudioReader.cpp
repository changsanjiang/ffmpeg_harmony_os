//
// Created on 2025/2/24.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "AudioReader.h"
#include <stdint.h>

namespace FFAV {

AudioReader::AudioReader(const std::string& url, int64_t start_time_pos_ms): url(url), start_time_pos_ms(start_time_pos_ms) {
    
}

AudioReader::~AudioReader() {
    stop();
}

void AudioReader::prepare() {
    std::unique_lock<std::mutex> lock(mtx);
    if ( flags.prepare_invoked || flags.has_error || flags.release_invoked ) {
        return;
    }
    
    flags.prepare_invoked = true;
    read_thread = std::make_unique<std::thread>(&AudioReader::ReadThread, this);
}

void AudioReader::start() {
    std::unique_lock<std::mutex> lock(mtx);
    if ( flags.is_started || flags.has_error || flags.release_invoked ) {
        return;
    }
    
    flags.is_started = true;
    lock.unlock();
    cv.notify_all();
}

void AudioReader::seek(int64_t time_pos_ms) {
    std::unique_lock<std::mutex> lock(mtx);
    if ( flags.has_error || flags.release_invoked ) {
        return;
    }
    
    if ( !flags.init_successful ) {
        start_time_pos_ms = time_pos_ms;
        return;    
    }
    
    if ( time_pos_ms < 0 ) time_pos_ms = 0;
    else if ( time_pos_ms > audio_stream_duration_ms ) time_pos_ms = audio_stream_duration_ms;
    
    int64_t time = av_rescale_q(time_pos_ms, (AVRational){ 1, 1000 }, AV_TIME_BASE_Q);
    
    // 拦截重复请求 
    if ( (flags.wants_seek && req_seek_time == time) || (time == seeking_time) ) {
#ifdef DEBUG
        client_print_message3("AAAA: AudioReader::seek: duplicate seek operation");
#endif
        return;
    }
    
    flags.wants_seek = true;
    req_seek_time = time;
    lock.unlock();
    cv.notify_all();
}

void AudioReader::stop() {
    std::unique_lock<std::mutex> lock(mtx);
    if ( flags.release_invoked ) {
        return;
    }
    
    flags.release_invoked = true;
    if ( audio_reader ) {
        audio_reader->interrupt();
    }
    
    lock.unlock();
    cv.notify_all();
    
    if ( read_thread && read_thread->joinable() ) {
        read_thread->join();
    }
    
    if ( audio_reader ) {
        delete audio_reader;
    }
}

void AudioReader::setPacketBufferFull(bool is_full) {
    is_pkt_buffer_full.store(is_full);
    cv.notify_all();
}

void AudioReader::setReadyToReadPacketCallback(AudioReader::ReadyToReadPacketCallback callback) {
    ready_to_read_pkt_callback = callback;
}

void AudioReader::setReadPacketCallback(AudioReader::ReadPacketCallback callback) {
    read_pkt_callback = callback;
}

void AudioReader::setErrorCallback(AudioReader::ErrorCallback callback) {
    error_callback = callback;
}

void AudioReader::ReadThread() {
    int ret = 0;
    {
        std::unique_lock<std::mutex> lock(mtx);
        if ( flags.release_invoked ) {
            return;
        }
        
        // init reader
        audio_reader = new MediaReader();
        lock.unlock();
        ret = audio_reader->open(url); // thread blocked;
        
        // re_lock
        lock.lock();
        if ( flags.release_invoked ) {
            return;
        }
        
        if ( ret < 0 ) {
            flags.has_error = true;
            lock.unlock();
            if ( error_callback ) error_callback(this, ret);
            return;
        }
        
        AVStream* audio_stream = audio_reader->getBestStream(AVMEDIA_TYPE_AUDIO);
        if ( audio_stream == nullptr ) {
            ret = AVERROR_STREAM_NOT_FOUND;
            flags.has_error = true;
            lock.unlock();
            if ( error_callback ) error_callback(this, ret);
            return;
        }
        
        audio_stream_index = audio_stream->index;
        audio_stream_time_base = audio_stream->time_base;
        audio_stream_duration_ms = av_rescale_q(audio_stream->duration, audio_stream_time_base, (AVRational){ 1, 1000 });
        
        if ( start_time_pos_ms > 0 ) {
            flags.wants_seek = true;
            req_seek_time = av_rescale_q(start_time_pos_ms, (AVRational){ 1, 1000 }, AV_TIME_BASE_Q);
        }
        
        // init successful
        flags.init_successful = true;
        lock.unlock();
        if ( ready_to_read_pkt_callback ) ready_to_read_pkt_callback(this, audio_stream);
    }
    
    // begin read pkts
    
    // handle seek & read pkts
    AVPacket* pkt = av_packet_alloc();
    bool should_seek;
    bool should_restart;
    bool should_exit;
    bool error_occurred;
    
    do {
restart:        
        ret = 0;
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
                
                if ( !flags.is_started ) {
                    return false;
                }
                
                if ( flags.wants_seek ) {
                    return true;
                }
                
                return !flags.is_read_eof && !is_pkt_buffer_full.load();
            });
            
            if ( flags.has_error || flags.release_invoked ) {
                should_exit = true;
            }
            else if ( flags.wants_seek ) {
                flags.wants_seek = false;
                seeking_time = req_seek_time;
                should_seek = true;
            }
        }
        
        if ( should_exit ) {
            goto exit_thread;
        }
        
        // handle seek
        if ( should_seek ) {
            ret = audio_reader->seek(seeking_time, -1); // thread blocked;
            
            // re_lock
            std::unique_lock<std::mutex> lock(mtx);
            if ( flags.has_error || flags.release_invoked ) {
                should_exit = true;
            }
            else if ( ret < 0 ) {
                if ( ret == AVERROR_EOF ) {
                    // nothing
                }
                else {
                    // error
                    error_occurred = true;
                    flags.has_error = true;
                    lock.unlock();
                    if ( error_callback ) error_callback(this, ret);
                }
            }
            else {
                // seek finished
                // reset flags
                flags.is_read_eof = false;
                
                if ( flags.wants_seek ) {
                    should_restart = true;
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
        ret = audio_reader->readPacket(pkt); // thread blocked;
        {
            std::unique_lock<std::mutex> lock(mtx);
            if ( flags.has_error || flags.release_invoked ) {
                should_exit = true;
            }
            else if ( ret < 0 ) {
                if ( ret == AVERROR_EOF ) {
                    bool should_flush = seeking_time != AV_NOPTS_VALUE; 
                    if ( should_flush ) {
                        seeking_time = AV_NOPTS_VALUE;
                    }
                    
                    // read eof
                    flags.is_read_eof = true;
                    
                    lock.unlock();
                    if ( read_pkt_callback ) read_pkt_callback(this, nullptr, should_flush);
                }
                else {
                    // error
                    error_occurred = true;
                    flags.has_error = true;
                    lock.unlock();
                    if ( error_callback ) error_callback(this, ret);
                }
            }
            else if ( pkt->stream_index == audio_stream_index ) {
                bool should_flush = seeking_time != AV_NOPTS_VALUE; 
                if ( should_flush ) {
                    seeking_time = AV_NOPTS_VALUE;
                }
                
                lock.unlock();
                if ( read_pkt_callback ) read_pkt_callback(this, pkt, should_flush);
            }
            
            if ( ret == 0 ) av_packet_unref(pkt);
        }
        
        if ( should_exit || error_occurred ) {
            goto exit_thread;
        }
    } while (true);
    
exit_thread:
    av_packet_free(&pkt);
    
#ifdef DEBUG
    client_print_message3("AAAA: read thread exit");
#endif
}

}