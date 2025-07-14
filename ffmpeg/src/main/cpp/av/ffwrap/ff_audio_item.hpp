//
// Created on 2025/4/28.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef FFAV_AudioItem_hpp
#define FFAV_AudioItem_hpp

#include <stdint.h>
#include <functional>
#include <string>
#include <map>
#include <mutex>
#include "ff_types.hpp"
#include "ff_audio_transcoder.hpp"

namespace FFAV {

class PacketReader;
class TaskScheduler;

class AudioItem {
    
public:
    struct Options {
    public:
        int64_t start_time_pos = 0; // in AV_TIME_BASE;
        std::map<std::string, std::string> http_options;
        
        int output_sample_rate = 44100;
        AVSampleFormat output_sample_format = AV_SAMPLE_FMT_FLTP;
        int output_channels = 2;
    };
    
    using StreamReadyCallback = std::function<void(int64_t duration, AVRational time_base)>;
    using BufferedTimeChangeCallback = std::function<void(int64_t buffered_time, AVRational time_base)>;
    using ErrorCallback = std::function<void(int ff_err)>;
    
    AudioItem(const std::string& url, const Options& options);
    virtual ~AudioItem();
    
    void prepare();
    void seekTo(int64_t time);  // in AV_TIME_BASE;
    /**
     * 尝试转码出指定数量的音频数据;
     *
     * 数据足够时返回值与 frameCapacity 一致;
     * 当 eof 时可能返回的样本数量小于指定的样本数量;
     * 如果未到 eof 数据不满足指定的样本数量时返回 0;
     *  */
    int tryTranscode(void **out_data, int frame_capacity, int64_t *out_pts, bool *out_eof); // out_pts in output time base;
    
    int getError();
    
    void setStreamReadyCallback(StreamReadyCallback callback);
    void setBufferedTimeChangeCallback(BufferedTimeChangeCallback callback);
    void setErrorCallback(ErrorCallback callback);

protected:
    virtual int onCreateTranscoder(StreamProvider* stream_provider, int output_sample_rate, AVSampleFormat output_sample_format, int output_channels, AudioTranscoder** out_transcoder);
    AudioTranscoder* getTranscoder();
    
private:
    void onStreamReady(PacketReader* reader);
    void onReadPacket(PacketReader* reader, AVPacket* pkt, bool should_flush);
    void onReadError(PacketReader* reader, int ff_err);
    
    void prepareReaderAgainIfError();
    
private:
    std::string _url;
    int64_t _start_time_pos;
    std::map<std::string, std::string> _http_options;
    int _output_sample_rate;
    AVSampleFormat _output_sample_format;
    int _output_channels;
    AVRational _output_time_base;
    AudioTranscoder* _transcoder { nullptr };
    
    PacketReader *_reader { nullptr };
    
    int64_t _duration { 0 }; // in output time base
    int64_t _buffered_time { 0 };  // in output time base
    std::atomic<int> _ff_err { 0 };
    
    std::mutex mtx;
    
    StreamReadyCallback _on_stream_ready_callback { nullptr };
    ErrorCallback _on_error_callback { nullptr };
    BufferedTimeChangeCallback _on_buffered_time_change_callback { nullptr };
    
    std::shared_ptr<TaskScheduler> _reset_reader_task { nullptr };
    
    int _network_status_change_callback_id;
    bool _initialized { false };
    bool _flush_packet_only { false };
    bool _seeking { false };
};

}

#endif //FFAV_AudioItem_hpp
