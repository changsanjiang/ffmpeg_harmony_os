//
// Created on 2025/1/7.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "MediaDecoder.h"
#include <sstream>

namespace CoreMedia {
    MediaDecoder::MediaDecoder(const std::string& url): reader(new MediaReader(url)), stream_index(-1), dec_ctx(nullptr), pkt(nullptr) {
    
    }

    MediaDecoder::~MediaDecoder() {
        release();
    }

    int MediaDecoder::prepare() {
        return reader->prepare();
    }

    int MediaDecoder::getStreamCount() {
        return reader->getStreamCount();
    }

    AVStream* _Nullable MediaDecoder::getStream(int stream_index) {
        return reader->getStream(stream_index);
    }

    int MediaDecoder::findBestStream(AVMediaType type) {
        return reader->findBestStream(type);
    }
    
    int MediaDecoder::getSelectedStreamIndex() {
        return stream_index;
    }

    int MediaDecoder::selectStream(int stream_index) {
        AVStream *stream = reader->getStream(stream_index);
        if ( stream == nullptr ) {
            return AVERROR_STREAM_NOT_FOUND;
        }
    
        // 获取解码器
        AVCodecParameters *codecpar = stream->codecpar;
        const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
        if ( codec == nullptr ) {
            return AVERROR_DECODER_NOT_FOUND; // 找不到解码器
        }
    
        // 创建解码器上下文
        dec_ctx = avcodec_alloc_context3(codec);
        if ( dec_ctx == nullptr ) {
            return AVERROR(ENOMEM);
        }
    
        // 初始化解码器上下文
        // Copy decoder parameters to decoder context
        int error = avcodec_parameters_to_context(dec_ctx, codecpar);
        if ( error < 0 ) {
            return error;
        }    
        
        // 打开解码器
        error = avcodec_open2(dec_ctx, codec, nullptr);
        if ( error < 0 ) {
            return error;
        }
        
        pkt = av_packet_alloc();
        if ( pkt == nullptr ) {
            return AVERROR(ENOMEM);
        }
    
        this->stream_index = stream_index;
        return 0;
    }

    int MediaDecoder::selectBestStream(AVMediaType type) {
        return selectStream(findBestStream(type));
    }
    
    std::string MediaDecoder::buildSrcArgs() {
        AVStream *stream = reader->getStream(getSelectedStreamIndex());
        if ( stream == nullptr || dec_ctx == nullptr ) {
            return nullptr;
        }
    
        switch(stream->codecpar->codec_type) {
            case AVMEDIA_TYPE_VIDEO: {
                std::stringstream src_ss;
                src_ss  << "video_size=" << dec_ctx->width << "x" << dec_ctx->height
                        << ":pix_fmt=" << dec_ctx->pix_fmt
                        << ":time_base=" << stream->time_base.num << "/" << stream->time_base.den
                        << ":pixel_aspect" << dec_ctx->sample_aspect_ratio.num << "/" << dec_ctx->sample_aspect_ratio.den;
                return src_ss.str();
            }
            case AVMEDIA_TYPE_AUDIO: {
                if ( dec_ctx->ch_layout.order == AV_CHANNEL_ORDER_UNSPEC ) av_channel_layout_default(&dec_ctx->ch_layout, dec_ctx->ch_layout.nb_channels);
                
                // get channel layout desc
                char ch_layout_desc[64];
                av_channel_layout_describe(&stream->codecpar->ch_layout, ch_layout_desc, sizeof(ch_layout_desc));
                
                std::stringstream src_ss;
                src_ss  << "time_base=" << stream->time_base.num << "/" << stream->time_base.den
                        << ":sample_rate=" << stream->codecpar->sample_rate
                        << ":sample_fmt=" << av_get_sample_fmt_name(static_cast<AVSampleFormat>(stream->codecpar->format))
                        << ":channel_layout=" << ch_layout_desc;
                return src_ss.str();
            }
            case AVMEDIA_TYPE_UNKNOWN:
            case AVMEDIA_TYPE_DATA:
            case AVMEDIA_TYPE_SUBTITLE:
            case AVMEDIA_TYPE_ATTACHMENT:
            case AVMEDIA_TYPE_NB:
                break;
        }
        return nullptr;
    }

    int MediaDecoder::decode(AVFrame* _Nonnull frame) {
        if ( stream_index == -1 ) {
            return AVERROR_STREAM_NOT_FOUND;
        }

        int error = 0;
        while(true) {
            error = avcodec_receive_frame(dec_ctx, frame); // receive decoded frames
            if ( error != AVERROR(EAGAIN) ) {
                break;
            }

            while( (error = reader->readPacket(pkt)) >= 0 ) { // read stream packet 
                if ( pkt->stream_index == stream_index ) { // selected stream
                    break;
                }
                av_packet_unref(pkt);
            }
            
            if ( error < 0 ) {
                break;
            }

            error = avcodec_send_packet(dec_ctx, pkt); // send packet to codec
            if ( error < 0 ) {
                break;
            }
            av_packet_unref(pkt);
        }
        return error;
    }

    int MediaDecoder::seek(int64_t timestamp, int flags) {
        int error = reader->seek(timestamp, flags);
        if ( error < 0 ) {
            return error;
        }

        if ( dec_ctx != nullptr ) {
            avcodec_flush_buffers(dec_ctx);
        }
        return 0;
    }

    void MediaDecoder::interrupt() {
        reader->interrupt();
    }

    void MediaDecoder::release() {
        if ( reader != nullptr ) {
            delete reader;
            reader = nullptr;
        }

        if ( pkt != nullptr ) {
            av_packet_free(&pkt);
        }
    
        if ( dec_ctx != nullptr ) {
            avcodec_free_context(&dec_ctx);
        }
    }
}