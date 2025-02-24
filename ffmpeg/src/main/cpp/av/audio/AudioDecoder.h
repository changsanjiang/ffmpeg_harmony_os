//
// Created on 2025/2/22.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef FFMPEG_HARMONY_OS_AUDIODECODER_H
#define FFMPEG_HARMONY_OS_AUDIODECODER_H

#include "av/core/MediaDecoder.h"
#include "av/core/FilterGraph.h"
#include "av/core/PacketQueue.h"
#include <thread>

namespace FFAV {

class AudioDecoder {
public:
    AudioDecoder();
    ~AudioDecoder();

    bool init(
        AVCodecParameters* audio_stream_codecpar,
        AVRational audio_stream_time_base,
        AVSampleFormat output_sample_fmt, 
        int output_sample_rate,
        std::string output_ch_layout_desc
    );
    void push(AVPacket* pkt, bool should_flush);
    void stop();
    
    // 设置缓冲已满
    // 当缓冲已满时将会暂停解码;
    void setSampleBufferFull(bool is_full);
    
    using DecodeFrameCallback = std::function<void(AudioDecoder* decoder, AVFrame* frame)>;
    void setDecodeFrameCallback(DecodeFrameCallback callback); // callback in locked;
    
    using PopPacketCallback = std::function<void(AudioDecoder* decoder, AVPacket* pkt)>;
    void setPopPacketCallback(PopPacketCallback callback);
    
    using ErrorCallback = std::function<void(AudioDecoder* decoder, int ff_err)>;
    void setErrorCallback(ErrorCallback callback);
    
private:
    AVBufferSrcParameters *buf_src_params;
    AVSampleFormat output_sample_fmt;
    int output_sample_rate;
    std::string output_ch_layout_desc;
    std::atomic<bool> is_sample_buffer_full;
    
    std::mutex mtx;
    std::condition_variable cv;
    
    PacketQueue* pkt_queue = new PacketQueue();
    MediaDecoder* audio_decoder { nullptr };
    FilterGraph* filter_graph { nullptr };
    std::unique_ptr<std::thread> dec_thread { nullptr };
    
    DecodeFrameCallback decode_frame_callback { nullptr };
    PopPacketCallback pop_pkt_callback { nullptr };
    ErrorCallback error_callback { nullptr };
    
    struct {
        unsigned int init_successful :1;
        unsigned int release_invoked :1;
        unsigned int has_error :1;
        unsigned int is_read_eof :1;
        unsigned int is_dec_eof :1;
    } flags;
    
    void DecThread();
    
    int initAudioDecoder(AVCodecParameters* codecpar);
    int initFilterGraph(
        const char* buf_src_name,
        const char* buf_sink_name,
        AVBufferSrcParameters *buf_src_params, 
        AVSampleFormat out_sample_fmt,
        int out_sample_rate,
        const std::string& out_ch_layout_desc
    );
    int resetFilterGraph();
};

}

#endif //FFMPEG_HARMONY_OS_AUDIODECODER_H
