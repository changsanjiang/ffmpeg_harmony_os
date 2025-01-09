//
// Created on 2025/1/7.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "MediaDecoder.h"
#include <sstream>

namespace CoreMedia {
    MediaDecoder::MediaDecoder(): dec_ctx(nullptr), time_base({0, 1}) {
    
    }

    MediaDecoder::~MediaDecoder() {
        release();
    }

    int MediaDecoder::prepare(AVStream* _Nonnull stream) {
        // 获取解码器
        AVCodecParameters* codecpar = stream->codecpar;
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
        
        time_base = stream->time_base;
        stream_index = stream->index;
        return 0;
    }

    int MediaDecoder::send(AVPacket* _Nonnull pkt) {
        if ( dec_ctx == nullptr ) {
            return AVERROR_INVALIDDATA;
        }
    
        if ( stream_index != pkt->stream_index ) {
            return AVERROR_INVALIDDATA;
        }
        
        return avcodec_send_packet(dec_ctx, pkt);
    }

    int MediaDecoder::receive(AVFrame* _Nonnull frame) {
        if ( dec_ctx == nullptr ) {
            return AVERROR_INVALIDDATA;
        }
        
        return avcodec_receive_frame(dec_ctx, frame);
    }
    
    void MediaDecoder::flush() {
        if ( dec_ctx != nullptr ) {
            avcodec_flush_buffers(dec_ctx);
        }
    }

    std::string MediaDecoder::makeSrcArgs() {
        if ( dec_ctx == nullptr ) {
            return nullptr;
        }
    
        switch(dec_ctx->codec_type) {
            case AVMEDIA_TYPE_VIDEO: {
                std::stringstream src_ss;
                src_ss  << "video_size=" << dec_ctx->width << "x" << dec_ctx->height
                        << ":pix_fmt=" << dec_ctx->pix_fmt
                        << ":time_base=" << time_base.num << "/" << time_base.den
                        << ":pixel_aspect" << dec_ctx->sample_aspect_ratio.num << "/" << dec_ctx->sample_aspect_ratio.den;
                return src_ss.str();
            }
            case AVMEDIA_TYPE_AUDIO: {
                if ( dec_ctx->ch_layout.order == AV_CHANNEL_ORDER_UNSPEC ) av_channel_layout_default(&dec_ctx->ch_layout, dec_ctx->ch_layout.nb_channels);
                
                // get channel layout desc
                char ch_layout_desc[64];
                av_channel_layout_describe(&dec_ctx->ch_layout, ch_layout_desc, sizeof(ch_layout_desc));
                
                std::stringstream src_ss;
                src_ss  << "time_base=" << time_base.num << "/" << time_base.den
                        << ":sample_rate=" << dec_ctx->sample_rate
                        << ":sample_fmt=" << av_get_sample_fmt_name(dec_ctx->sample_fmt)
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

    void MediaDecoder::release() {
        if ( dec_ctx != nullptr ) {
            avcodec_free_context(&dec_ctx);
        }
    }
}