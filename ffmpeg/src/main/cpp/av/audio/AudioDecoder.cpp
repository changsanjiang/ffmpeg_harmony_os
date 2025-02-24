//
// Created on 2025/2/22.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "AudioDecoder.h"
#include "av/core/AudioUtils.h"
#include <memory>
#include <sstream>
#include <stdint.h>

namespace FFAV {

const char* FILTER_BUFFER_SRC_NAME = "0:a";
const char* FILTER_BUFFER_SINK_NAME = "outa";

AudioDecoder::AudioDecoder() {
    
}

AudioDecoder::~AudioDecoder() {
    stop();
}

bool AudioDecoder::init(
    AVCodecParameters* audio_stream_codecpar,
    AVRational audio_stream_time_base,
    AVSampleFormat output_sample_fmt, 
    int output_sample_rate,
    int output_nb_channels,
    std::string output_ch_layout_desc,
    int maximum_samples_threshold
) {
    std::unique_lock<std::mutex> lock(mtx);
    if ( flags.has_error || flags.release_invoked ) {
        return false;
    }
    
    this->output_sample_fmt = output_sample_fmt;
    this->output_sample_rate = output_sample_rate;
    this->output_ch_layout_desc = output_ch_layout_desc;
    this->maximum_samples_threshold = maximum_samples_threshold;
    
    int ret = initAudioDecoder(audio_stream_codecpar);
    if ( ret < 0 ) {
        goto exit_init;
    }
    
    buf_src_params = audio_decoder->createBufferSrcParameters(audio_stream_time_base);
    ret = resetFilterGraph();
    if ( ret < 0 ) {
        goto exit_init;
    }
    
    ret = initAudioFifo(output_sample_fmt, output_nb_channels);
    if ( ret < 0 ) {
        goto exit_init;
    }
    
    dec_thread = std::make_unique<std::thread>(&AudioDecoder::DecThread, this);
    
exit_init: 
    if ( ret < 0 ) {
        flags.has_error = true;
        lock.unlock();
        if ( error_callback ) error_callback(this, ret);
        return false;
    }
    return true;
}

void AudioDecoder::push(AVPacket* pkt, bool should_flush) {
    std::unique_lock<std::mutex> lock(mtx);
    if ( flags.has_error || flags.release_invoked ) {
        return;
    }

    if ( should_flush ) {
        audio_decoder->flush();
        audio_fifo->clear();
        resetFilterGraph();
    }
    
    if ( pkt ) {
        if ( flags.is_read_eof ) {
            flags.is_read_eof = false;
        }
        pkt_queue->push(pkt);
        lock.unlock();
        if ( pkt_size_change_callback ) pkt_size_change_callback(this);
        cv.notify_all();
    }
    else {
        flags.is_read_eof = true;
        lock.unlock();
        cv.notify_all();
    }
}

void AudioDecoder::stop() {
    
}

int64_t AudioDecoder::getBufferedPacketSize() {
    std::unique_lock<std::mutex> lock(mtx);
    return pkt_queue->getSize();
}

void AudioDecoder::setDecodedFramesChangeCallback(AudioDecoder::DecodedFramesChangeCallback callback) {
    decoded_frames_change_callback = callback;
}

void AudioDecoder::setErrorCallback(AudioDecoder::ErrorCallback callback) {
    error_callback = callback;
}

void AudioDecoder::setBufferedPacketSizeChangeCallback(AudioDecoder::BufferedPacketSizeChangeCallback callback) {
    pkt_size_change_callback = callback;
}

void AudioDecoder::DecThread() {
    int ret = 0;
    AVPacket *pkt = av_packet_alloc();
    AVFrame *dec_frame = av_frame_alloc();
    AVFrame *filt_frame = av_frame_alloc();
    bool should_exit;
    bool error_occurred;
    
    do {
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
                if ( audio_fifo->getNumberOfSamples() < maximum_samples_threshold ) {
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
                ret = AudioUtils::transcode(pkt, audio_decoder, dec_frame, filter_graph, filt_frame, FILTER_BUFFER_SRC_NAME, FILTER_BUFFER_SINK_NAME, audio_fifo);
                av_packet_unref(pkt);
                
                // error
                if ( ret < 0 && ret != AVERROR(EAGAIN) ) {
                    error_occurred = true;
                    flags.has_error = true;
                    lock.unlock();
                    if ( error_callback ) error_callback(this, ret);
                }
                else {
                    lock.unlock();
                    if ( pkt_size_change_callback ) pkt_size_change_callback(this);
                    if ( decoded_frames_change_callback ) decoded_frames_change_callback(this);
                }
            }
            // read eof
            else if ( flags.is_read_eof ) {
#ifdef DEBUG
                client_print_message3("AAAA: dec eof, fifo size: %ld", audio_fifo->getSize());
#endif
                ret = AudioUtils::transcode(nullptr, audio_decoder, dec_frame, filter_graph, filt_frame, FILTER_BUFFER_SRC_NAME, FILTER_BUFFER_SINK_NAME, audio_fifo);
                
                // dec eof
                if ( ret == AVERROR_EOF ) {
                    flags.is_dec_eof = true;
                    lock.unlock();
                    if ( decoded_frames_change_callback ) decoded_frames_change_callback(this);
                }
                else if ( ret < 0 ) {
                    error_occurred = true;
                    flags.has_error = true;
                    lock.unlock();
                    if ( error_callback ) error_callback(this, ret);
                }
            }
        }
        
        if ( should_exit || error_occurred ) {
            goto exit_thread;
        }
    } while (true);
    
exit_thread:
    av_packet_free(&pkt);
    av_frame_free(&dec_frame);
    av_frame_free(&filt_frame);
    
#ifdef DEBUG
    client_print_message3("AAAA: dec thread exit");
#endif
}

int AudioDecoder::initAudioDecoder(AVCodecParameters* codecpar) {
    int ff_ret = 0;
    audio_decoder = new MediaDecoder();
    ff_ret = audio_decoder->init(codecpar);
    if ( ff_ret < 0 ) {
        return ff_ret;
    }
    return 0;
}

int AudioDecoder::initFilterGraph(
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

int AudioDecoder::resetFilterGraph() {
    return initFilterGraph(FILTER_BUFFER_SRC_NAME, FILTER_BUFFER_SINK_NAME, buf_src_params, output_sample_fmt, output_sample_rate, output_ch_layout_desc);
}

int AudioDecoder::initAudioFifo(
    AVSampleFormat sample_fmt, 
    int nb_channels
) {
    int ff_ret = 0;
    
    // init audio fifo buffer
    audio_fifo = new AudioFifo();
    ff_ret = audio_fifo->init(sample_fmt, nb_channels, 1);
    if ( ff_ret < 0 ) {
        return ff_ret;
    }

    return 0;
}

}