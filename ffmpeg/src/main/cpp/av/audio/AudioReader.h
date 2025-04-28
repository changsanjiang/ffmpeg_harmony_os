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
    AudioReader(const std::string& url, const std::map<std::string, std::string>& http_options);
    ~AudioReader();
    
    void prepare(int64_t start_time_pos_ms = AV_NOPTS_VALUE);
    void start();
    void seek(int64_t time_pos_ms);
    void stop();
    void reset(); // 仅限报错后调用, 调用该方法重置, 可重新调用 prepare 准备; 注意: stop 后调用重置不生效;
    
    // 设置缓冲是否已满;
    // 当缓冲已满时将会暂停读取;
    void setPacketBufferFull(bool is_full);
    
    using ReadyToReadCallback = std::function<void(AudioReader* reader, AVStream* stream)>;
    void setReadyToReadCallback(ReadyToReadCallback callback);
    
    // seek 之后， read pkt 之前 Queue中的数据都可以使用
    // read pkt 之后(should_flush == true)， queue 中的数据需要清空， 用来存放新位置的 pkt;
    using ReadPacketCallback = std::function<void(AudioReader* reader, AVPacket* pkt, bool should_flush)>;
    void setReadPacketCallback(ReadPacketCallback callback);
    
    using ReadErrorCallback = std::function<void(AudioReader* reader, int ff_err)>;
    void setReadErrorCallback(ReadErrorCallback callback);
    
private:
    const std::string url;
    const std::map<std::string, std::string> http_options;
    
    std::mutex mtx;
    std::condition_variable cv;

    MediaReader* audio_reader { nullptr };
    std::unique_ptr<std::thread> read_thread { nullptr };
     
    int audio_stream_index;
    
    std::atomic<bool> is_pkt_buffer_full { false };

    int64_t req_seek_time { AV_NOPTS_VALUE }; // in base q; 
    int64_t seeking_time { AV_NOPTS_VALUE }; // in base q; 
    
    ReadyToReadCallback ready_to_read_callback { nullptr };
    ReadPacketCallback read_pkt_callback { nullptr };
    ReadErrorCallback error_callback { nullptr };

    struct {
        unsigned int release_invoked :1;
        unsigned int prepare_invoked :1;
        unsigned int has_error :1;

        unsigned int is_started :1;
        unsigned int is_read_eof :1;
    } flags { 0 };
    
    void ReadThread();
};

}

#endif //FFMPEG_HARMONY_OS_AUDIOREADER_H
