//
//  SingleStreamAudioTranscoder.cpp
//  LWZFFmpegLib
//
//  Created by sj on 2025/5/16.
//

#include "ff_single_stream_audio_transcoder.hpp"
#include "ff_packet_queue.hpp"
#include "ff_media_decoder.hpp"
#include "ff_filter_graph.hpp"
#include "ff_audio_fifo.hpp"
#include "ff_includes.hpp"
#include "ff_audio_utils.hpp"

#include <sstream>
#include <stdint.h>

namespace FFAV {

/// 数据包缓存阈值(字节)
static int const kPacketSizeThreshold = 1 * 1024 * 1024;
static const std::string FF_FILTER_BUFFER_SRC_NAME = "0:a";
static const std::string FF_FILTER_BUFFER_SINK_NAME = "result";

SingleStreamAudioTranscoder::SingleStreamAudioTranscoder(): AudioTranscoder() {
    
}

SingleStreamAudioTranscoder::~SingleStreamAudioTranscoder() {
    if ( _decoder ) {
        delete _decoder;
        _decoder = nullptr;
    }
    
    if ( _buf_src_params ) {
        av_free(_buf_src_params);
        _buf_src_params = nullptr;
    }
    
    if ( _filter_graph ) {
        delete _filter_graph;
        _filter_graph = nullptr;
    }
    
    if ( _packet_queue ) {
        delete _packet_queue;
        _packet_queue = nullptr;
    }
    
    if ( _fifo ) {
        delete _fifo;
        _fifo = nullptr;
    }
    
    if ( _pkt ) {
        av_packet_free(&_pkt);
    }
    
    if ( _dec_frame ) {
        av_frame_free(&_dec_frame);
    }
    
    if ( _filt_frame ) {
        av_frame_free(&_filt_frame);
    }
}

int SingleStreamAudioTranscoder::init(StreamProvider* stream_provider, int output_sample_rate, AVSampleFormat output_sample_format, int output_channels) {
    auto stream = stream_provider->getBestStream(AVMEDIA_TYPE_AUDIO) ?: stream_provider->getFirstStream(AVMEDIA_TYPE_AUDIO);
    return init(stream, output_sample_rate, output_sample_format, output_channels);    
}

int SingleStreamAudioTranscoder::init(AVStream* in_stream, int output_sample_rate, AVSampleFormat output_sample_format, int output_channels) {
    if ( !in_stream ) {
        return AVERROR_STREAM_NOT_FOUND;
    }
    
    int ret = 0;
    char ch_layout_desc[64];

    _in_stream_time_base = in_stream->time_base;
    _in_stream_index = in_stream->index;
    _duration = av_rescale_q(in_stream->duration, in_stream->time_base, (AVRational){ 1, output_sample_rate });
    _output_sample_rate = output_sample_rate;
    _output_sample_format = output_sample_format;
    _output_channels = output_channels;
    _output_bytes_per_sample = av_get_bytes_per_sample(output_sample_format);
    _output_interleaved = av_sample_fmt_is_planar(output_sample_format) == 0;
    
    AVChannelLayout output_channel_layout;
    av_channel_layout_default(&output_channel_layout, _output_channels);
    av_channel_layout_describe(&output_channel_layout, ch_layout_desc, sizeof(ch_layout_desc)); // get channel layout desc
    _output_channel_layout_desc = ch_layout_desc;

    // init decoder
    _decoder = new FFAV::MediaDecoder();
    ret = _decoder->init(in_stream->codecpar);
    if ( ret < 0 ) {
        goto on_exit;
    }
    
    // init filter graph
    _buf_src_params = _decoder->createBufferSrcParameters(in_stream->time_base);

    ret = recreateFilterGraph();
    if ( ret < 0 ) {
        goto on_exit;
    }
    
    // init packet queue
    _packet_queue = new PacketQueue();
    
    // init audio fifo
    _fifo = new AudioFifo();
    ret = _fifo->init(output_sample_rate, output_sample_format, output_channels, 1);
    if ( ret < 0 ) {
        goto on_exit;
    }
    
    _pkt = av_packet_alloc();
    _dec_frame = av_frame_alloc();
    _filt_frame = av_frame_alloc();
    _initialized = true;
    
on_exit:
    return ret;
}

int SingleStreamAudioTranscoder::enqueue(AVPacket * _Nullable packet) {
    if ( packet ) {
        if ( packet->stream_index == _in_stream_index ) {
            _pkt_queue_end_pts = av_rescale_q(packet->pts + packet->duration, _in_stream_time_base, (AVRational){ 1, _output_sample_rate });
            _packet_queue->push(packet);
        }
    }
    else {
        _packet_reached_eof = true;
        
        if ( _packet_queue->getCount() == 0 ) {
            _transcoding_eof = true;
        }
    }
    return 0;
}

int SingleStreamAudioTranscoder::flush(FlushMode mode) {
    switch ( mode ) {
        case FlushMode::Full: {
            _packet_reached_eof = false;
            _transcoding_eof = false;
            _should_drain_packets = false;
            
            _packet_queue->clear();
            _decoder->flush();
            _fifo->clear();
            
            int ret = recreateFilterGraph();
            if ( ret < 0 ) {
                return ret;
            }
        }
            break;
        case FlushMode::PacketOnly: {
            _packet_reached_eof = false;
            _transcoding_eof = false;
            _should_drain_packets = false;
            
            // 清理pkt相关的缓存;
            _packet_queue->clear();
            _decoder->flush();
            
            // 保留fifo的缓存, 当有新的pkt进行转码时需要在转码回调中对齐到fifo;
            _should_align_frames = _fifo->getNumberOfSamples() > 0;
            
            int ret = recreateFilterGraph();
            if ( ret < 0 ) {
                return ret;
            }
        }
            break;
    }
    return 0;
}

int SingleStreamAudioTranscoder::tryTranscode(void * _Nonnull * _Nonnull out_data, int frame_capacity, int64_t * _Nullable out_pts, bool * _Nullable out_eof) {
    if ( !_initialized ) {
        return false;
    }

    int ret = process(frame_capacity);
    // 只有数据量足够或者eof时才进行读取操作;
    // 存在数据时: 判断是否 eof 或者数据量足够;
    if ( ret > 0 && (ret >= frame_capacity || _transcoding_eof) ) {
        return read(out_data, frame_capacity, out_pts, out_eof);
    }

    if ( out_eof ) {
        *out_eof = isReadEof();
    }
    return 0;
}

int SingleStreamAudioTranscoder::process(int frame_capacity) {
    if ( !_initialized ) {
        return false;
    }

    // 控制缓冲， 确保可以流畅播放(队列中的pkts加起来的时长需要满足3s)
    if ( !_should_drain_packets ) {
        if ( _packet_reached_eof ) {
            _should_drain_packets = true;
        }
        else {
            int64_t duration = _packet_queue->getDuration();
            if ( duration >= av_rescale_q(3, (AVRational){ 1, 1 }, _in_stream_time_base) ) {
                _should_drain_packets = true; // 缓冲大于3秒时设置需要榨干队列中的pkts;
            }
        }
    }

    if ( !_should_drain_packets ) {
        return _fifo->getNumberOfSamples();
    }

    // transcoding
    // return if err
    int ret = 0;
    do {
        // 如果转码后的数据足够(满足frame_capacity)或者已转码结束, 则退出循环
        if ( _fifo->getNumberOfSamples() >= frame_capacity || _transcoding_eof ) {
            break;
        }
        
        // 当前无可转码数据时, 退出循环
        if ( !_packet_reached_eof && _packet_queue->getSize() == 0 ) {
            break;
        }
        
        // `pkt reached eof`或队列中有可转码的数据
        AVPacket *next = nullptr;
        if ( _packet_queue->pop(_pkt) ) {
            next = _pkt;
        }
        
        // transcode
        ret = FFAV::AudioUtils::processPacket(next, _decoder, _dec_frame, _filter_graph, FF_FILTER_BUFFER_SRC_NAME, FF_FILTER_BUFFER_SINK_NAME, _filt_frame, [&](AVFrame *filt_frame) {
            // 需要对齐
            if ( _should_align_frames ) {
                int64_t aligned_pts = _fifo->getEndPts();
                int64_t frame_start_pts = filt_frame->pts;
                int64_t frame_end_pts = frame_start_pts + filt_frame->nb_samples;
                if ( aligned_pts >= frame_end_pts ) {
                    return 0;
                }

                if ( frame_start_pts < aligned_pts ) {
                    // intersecting samples
                    int64_t skip_samples = aligned_pts - frame_start_pts;
                    int64_t remain_samples = frame_end_pts - aligned_pts;

                    // LR LR LR
                    if ( _output_interleaved ) {
                        int64_t ptr_offset = skip_samples * _output_bytes_per_sample * _output_channels;
                        uint8_t *ptr = filt_frame->data[0] + ptr_offset;
                        _should_align_frames = false;
                        return _fifo->write((void **)&ptr, (int)remain_samples, aligned_pts);
                    }
                    // ch0: L L L
                    // ch1: R R R
                    else {
                        int64_t ptr_offset = skip_samples * _output_bytes_per_sample;
                        uint8_t *chPtr[_output_channels];
                        for (int ch = 0; ch < _output_channels; ++ch) {
                            chPtr[ch] = filt_frame->data[ch] + ptr_offset;
                        }
                        _should_align_frames = false;
                        return _fifo->write((void **)chPtr, (int)remain_samples, aligned_pts);
                    }
                }
                else {
                    _should_align_frames = false;
                }
            }
            return _fifo->write((void **)filt_frame->data, filt_frame->nb_samples, filt_frame->pts);
        });
        
        if ( next ) {
            av_packet_unref(next);
        }
        
        // eof
        if ( ret == AVERROR_EOF ) {
            _transcoding_eof = true;
            break;
        }
        else if ( ret == AVERROR(EAGAIN) ) {
            // continue transcode
        }
        else if ( ret == AVERROR_INVALIDDATA ) {
            // continue transcode
        }
        else if ( ret < 0 ) {
            // transcode error
            return ret; // return error;
        }
    } while (true);

    if ( _packet_queue->getCount() == 0 ) {
        // 已榨干pkts
        _should_drain_packets = false;
    }

    // 返回已转码的数量
    return _fifo->getNumberOfSamples();
}

int SingleStreamAudioTranscoder::read(void *_Nonnull*_Nonnull out_data, int frame_capacity, int64_t *_Nullable out_pts, bool *_Nullable out_eof) {
    if ( !_initialized ) {
        return false;
    }

    int ret = 0;
    // read fifo
    int nb_samples = _fifo->getNumberOfSamples();
    if ( nb_samples > 0 ) {
        ret = _fifo->read(out_data, frame_capacity, out_pts);
    }
    
    // eof flag
    if ( out_eof ) {
        *out_eof = isReadEof();
    }
    return ret;
}

bool SingleStreamAudioTranscoder::isReadEof() {
    return _initialized && _transcoding_eof && _fifo->getNumberOfSamples() == 0; // 转码eof并且所有数据均被读取;
}

bool SingleStreamAudioTranscoder::isTranscodingEof() {
    return _transcoding_eof;
}

bool SingleStreamAudioTranscoder::isPacketBufferFull() {
    return _initialized && _packet_queue->getSize() >= kPacketSizeThreshold;
}

int64_t SingleStreamAudioTranscoder::getPacketQueueEndPts() {
    return _pkt_queue_end_pts;
}

int64_t SingleStreamAudioTranscoder::getFifoEndPts() {
    return _initialized ? _fifo->getEndPts() : 0;
}

int64_t SingleStreamAudioTranscoder::getDuration() {
    return _duration;
}

int SingleStreamAudioTranscoder::getOutputSampleRate() {
    return _output_sample_rate;
}

AVSampleFormat SingleStreamAudioTranscoder::getOutputSampleFormat() {
    return _output_sample_format;
}

int SingleStreamAudioTranscoder::getOutputChannels() {
    return _output_channels;
}

int SingleStreamAudioTranscoder::recreateFilterGraph() {
    if ( _filter_graph ) {
        delete _filter_graph;
        _filter_graph = nullptr;
    }
    return createFilterGraph(_buf_src_params, _output_sample_rate, _output_sample_format, _output_channel_layout_desc, &_filter_graph);
}

int SingleStreamAudioTranscoder::createFilterGraph(AVBufferSrcParameters *_Nonnull buf_src_params, int output_sample_rate, AVSampleFormat output_sample_format, const std::string& output_channel_layout_desc, FFAV::FilterGraph *_Nullable*_Nonnull out_filter_graph) {
    FFAV::FilterGraph *filter_graph = new FFAV::FilterGraph();
    std::stringstream filter_desc;
    int ret = 0;
    
    ret = filter_graph->init();
    if ( ret < 0 ) {
        goto on_exit;
    }
    
    ret = filter_graph->addBufferSourceFilter(FF_FILTER_BUFFER_SRC_NAME, AVMEDIA_TYPE_AUDIO, buf_src_params);
    if ( ret < 0 ) {
        goto on_exit;
    }
    
    ret = filter_graph->addAudioBufferSinkFilter(FF_FILTER_BUFFER_SINK_NAME, output_sample_rate, output_sample_format, output_channel_layout_desc);
    if ( ret < 0 ) {
        goto on_exit;
    }
    
    filter_desc << "[" << FF_FILTER_BUFFER_SRC_NAME << "]"
                << "aformat=sample_fmts=" << av_get_sample_fmt_name(output_sample_format) << ":channel_layouts=" << output_channel_layout_desc << ",aresample=" << output_sample_rate
                << "[" << FF_FILTER_BUFFER_SINK_NAME << "]";

    ret = filter_graph->parse(filter_desc.str());
    
    if ( ret < 0 ) {
        goto on_exit;
    }
    
    ret = filter_graph->configure();
    if ( ret < 0 ) {
        goto on_exit;
    }
    
    
on_exit:
    if ( ret < 0 ) {
        if ( filter_graph ) {
            delete filter_graph;
        }
    }
    else {
        *out_filter_graph = filter_graph;
    }

    return ret;
}

}
