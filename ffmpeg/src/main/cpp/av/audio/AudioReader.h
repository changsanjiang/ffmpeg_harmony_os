//
// Created on 2025/2/24.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef FFMPEG_HARMONY_OS_AUDIOREADER_H
#define FFMPEG_HARMONY_OS_AUDIOREADER_H

#include "av/core/MediaReader.h"
#include <stdint.h>
#include <thread>
#include <string>

namespace FFAV {

class AudioReader {
public:
    AudioReader(const std::string& url, int64_t start_time_pos_ms);
    ~AudioReader();
    
    void prepare();
    void seek(int64_t time_pos_ms);
    void stop();
    void setBufferedPacketSize(int64_t size);
    
    // seek 之后， read pkt 之前 Queue中的数据都可以使用
    // read pkt 之后(should_flush == true)， queue 中的数据需要清空， 用来存放新位置的 pkt;
    using ReadPacketCallback = std::function<void(AudioReader* reader, AVPacket* pkt, bool should_flush)>;
    void setReadPacketCallback(ReadPacketCallback callback);
    
    using ErrorCallback = std::function<void(AudioReader* reader, int ff_err)>;
    void setErrorCallback(ErrorCallback callback);
    
private:
    const std::string url;
    int64_t start_time_pos_ms;
    
    std::mutex mtx;
    std::condition_variable cv;

    MediaReader* audio_reader { nullptr };
    std::unique_ptr<std::thread> read_thread { nullptr };
     
    int audio_stream_index;
    AVRational audio_stream_time_base;
    int64_t audio_stream_duration_ms;
    
    int64_t pkt_size_threshold { 5 * 1024 * 1024 }; // in bytes, 5M;
    std::atomic<int64_t> pkt_size_bufferred;

    int64_t req_seek_time; // in base q;
    int64_t seeking_time { AV_NOPTS_VALUE }; // in base q; current seek time; 
    
    ReadPacketCallback read_pkt_callback { nullptr };
    ErrorCallback error_callback { nullptr };

    struct {
        unsigned int init_successful :1;
        unsigned int release_invoked :1;
        unsigned int prepare_invoked :1;
        unsigned int has_error :1;

        unsigned int wants_seek :1;
        unsigned int is_read_eof :1;
    } flags { 0 };
    
    void ReadThread();
};

}

#endif //FFMPEG_HARMONY_OS_AUDIOREADER_H
