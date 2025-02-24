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
#include "av/core/AudioFifo.h"
#include <stdint.h>
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
        int output_nb_channels,
        std::string output_ch_layout_desc,
        int maximum_samples_threshold
    );
    void push(AVPacket* pkt, bool should_flush);
    void stop();
    
    int64_t getBufferedPacketSize();
    int getNumberOfDecodedSamples();
    int64_t getNextPts(); // in output timebase;
    int read(void** data, int nb_read_samples, int64_t* pts_ptr);
    
    using DecodedSamplesChangeCallback = std::function<void(AudioDecoder* decoder)>;
    void setDecodedSamplesChangeCallback(DecodedSamplesChangeCallback callback);
     
    using BufferedPacketSizeChangeCallback = std::function<void(AudioDecoder* decoder)>;
    void setBufferedPacketSizeChangeCallback(BufferedPacketSizeChangeCallback callback);
    
    using ErrorCallback = std::function<void(AudioDecoder* decoder, int ff_err)>;
    void setErrorCallback(ErrorCallback callback);
    
private:
    AVBufferSrcParameters *buf_src_params;
    AVSampleFormat output_sample_fmt;
    int output_sample_rate;
    std::string output_ch_layout_desc;
    int maximum_samples_threshold;
    
    std::mutex mtx;
    std::condition_variable cv;
    
    PacketQueue* pkt_queue = new PacketQueue();
    MediaDecoder* audio_decoder { nullptr };
    FilterGraph* filter_graph { nullptr };
    AudioFifo* audio_fifo { nullptr };
    std::unique_ptr<std::thread> dec_thread { nullptr };
    
    DecodedSamplesChangeCallback decoded_samples_change_callback { nullptr };
    BufferedPacketSizeChangeCallback pkt_size_change_callback { nullptr };
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
    int initAudioFifo(
        AVSampleFormat sample_fmt, 
        int nb_channels
    );
};

}

#endif //FFMPEG_HARMONY_OS_AUDIODECODER_H
