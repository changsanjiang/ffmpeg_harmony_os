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
//  ff_packet_reader.hpp
//  LWZFFmpegLib
//
//  Created by sj on 2025/5/16.
//

#ifndef FFAV_PacketReader_hpp
#define FFAV_PacketReader_hpp

#include <functional>
#include <map>
#include <thread>
#include <mutex>
#include "ff_stream_provider.hpp"
#include "ff_types.hpp"

namespace FFAV {

class MediaReader;

/** 用来读取未解码的数据包; */
class PacketReader {
    
public:
    PacketReader();
    ~PacketReader();
    
    // normal: prepare => start => stop
    // error: reset => prepare(reprepare) => start => stop
    
    void prepare(const std::string& url, const std::map<std::string, std::string>& http_options = {}); // 启动线程异步打开音频流, 等待启动读取数据包; seek_position in base q;
    void start(); // 启动读取数据包;
    void seekTo(int64_t seek_position); // seek_position in base q;
    void stop(); // 停止读取数据包;
    void reset(); // 重置所有状态, 重置后可以重新调用 prepare 初始化; 可能会阻塞调用线程(会等待内部的读取线程执行结束);
    
    void setPacketBufferFull(bool is_full);
    
    using StreamReadyCallback = std::function<void(PacketReader *_Nonnull reader)>;
    void setStreamReadyCallback(StreamReadyCallback callback); // 打开流的回调;
    
    using ReadPacketCallback = std::function<void(PacketReader *_Nonnull reader, AVPacket *_Nullable packet, bool should_flush)>; // eof 时 packet 为 nullptr;
    void setReadPacketCallback(ReadPacketCallback callback); // 读取到数据包的回调;
    
    using ErrorCallback = std::function<void(PacketReader *_Nonnull reader, int ff_err)>;
    void setErrorCallback(ErrorCallback callback); // 报错时的回调;
    
    int getError();
    
    // 请在 StreamReady 的时候调用
    StreamProvider*_Nullable getStreamProvider();
    int getStreamCount();
    AVStream*_Nullable*_Nullable getStreams();
    AVStream*_Nullable getBestStream(AVMediaType mediaType);
    AVStream*_Nullable getFirstStream(AVMediaType mediaType);
    
private:
    void ReadLoop();
    bool checkSeekReq(); // 需要seek时返回true;
    
private:
    enum class State {
        Pending,
        Reading,
        Stopped
    };
    
    std::unique_ptr<std::thread> _read_thread;
    std::mutex _mtx;
    std::condition_variable _cv;
    
    StreamReadyCallback _on_audio_stream_ready_callback;
    ReadPacketCallback _on_read_packet_callback;
    ErrorCallback _on_error_callback;
    
    std::string _url;
    std::map<std::string, std::string> _http_options;
    MediaReader *_Nullable _media_reader { nullptr };
    
    std::atomic<int64_t> _req_seek_time { AV_NOPTS_VALUE }; // in base q;
    int64_t _seeking_time { AV_NOPTS_VALUE }; // in base q;
    
    std::atomic<State> _state { State::Pending };
    std::atomic<bool> _packet_buffer_full { false };
    std::atomic<int> _ff_err { 0 };
    bool _reached_eof { false };
};

}

#endif /* FFAV_PacketReader_hpp */
