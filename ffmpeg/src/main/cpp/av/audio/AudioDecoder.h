//
// Created on 2025/2/22.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef FFMPEG_HARMONY_OS_AUDIODECODER_H
#define FFMPEG_HARMONY_OS_AUDIODECODER_H

#include "Error.h"
#include "av/core/MediaReader.h"
#include "av/core/MediaDecoder.h"
#include "av/core/FilterGraph.h"
#include "av/core/AudioFifo.h"
#include "av/core/PacketQueue.h"
#include <string>
#include <thread>

namespace FFAV {

class AudioDecoder {
public:
    struct Options {
        int64_t start_time_position_ms;
        AVSampleFormat output_sample_fmt;
    };
    
    AudioDecoder(const std::string& url, Options opts);
    ~AudioDecoder();

    void prepare();
    void seek(int64_t time_pos_ms);
    int  read(void** data, int nb_samples, int64_t* pts); 
    void stop();
    
    int getSize(); // 获取当前已解码的帧数 nb_samples;
    
    using ErrorCallback = std::function<void(std::shared_ptr<Error> error)>;
    void setErrorCallback(ErrorCallback callback);
    
private:
    const std::string url;
    const int64_t start_time_position_ms;
    
    std::mutex mtx;
    std::condition_variable cv;
    
    MediaReader* audio_reader = nullptr;
    MediaDecoder* audio_decoder = nullptr;
    FilterGraph* filter_graph = nullptr;
    PacketQueue* pkt_queue = nullptr;
    AudioFifo* audio_fifo = nullptr;
    
    ErrorCallback error_callback = nullptr;
    
    std::unique_ptr<std::thread> read_thread { nullptr };
    std::unique_ptr<std::thread> dec_thread { nullptr };
    
    struct {
        unsigned int release_invoked :1;
        
    } flags { 0 };
    
    void ReadThread();
    void DecThread();
    
    void init();
};

}

#endif //FFMPEG_HARMONY_OS_AUDIODECODER_H
