//
// Created on 2025/7/4.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef FFAV_SingleStreamAudioTranscoder_hpp
#define FFAV_SingleStreamAudioTranscoder_hpp

#include "ff_types.hpp"
#include "ff_audio_transcoder.hpp"
#include <stdint.h>
#include <string>

namespace FFAV {
class MediaDecoder;
class FilterGraph;
class PacketQueue;
class AudioFifo;

class SingleStreamAudioTranscoder: public AudioTranscoder {
public:
    SingleStreamAudioTranscoder();
    ~SingleStreamAudioTranscoder();
    
    // 自动选择一个音频流进行转码
    int init(StreamProvider*_Nonnull stream_provider, int output_sample_rate, AVSampleFormat output_sample_format, int output_channels);
    // 指定音频流进行转码
    int init(AVStream*_Nonnull in_stream, int output_sample_rate, AVSampleFormat output_sample_format, int output_channels);

    int enqueue(AVPacket *_Nullable packet);
    int flush(FlushMode mode = FlushMode::Full);

    int tryTranscode(void *_Nonnull*_Nonnull out_data, int frame_capacity, int64_t *_Nullable out_pts, bool *_Nullable out_eof);

    bool isPacketBufferFull();
    
    /// 尝试转码出指定数量的音频数据, 该方法不读取数据, 仅进行转码处理;
    ///
    /// 数据足够时返回值可能大于等于 frame_capacity;
    /// 数据不足时返回值可能小于 frame_capacity;
    /// 
    /// 出错时返回ff_err
    int process(int frame_capacity);
    /// 读取转码的数据;
    ///
    /// 如果存在转码数据, 就读取到 out_data 中;
    /// 读取到的数据可能 <= frame_capacity;
    /// out_eof: 是否读取到结尾了
    /// 出错时返回ff_err
    int read(void *_Nonnull*_Nonnull out_data, int frame_capacity, int64_t *_Nullable out_pts, bool *_Nullable out_eof);
    
    
    /// 是否读取到结尾了
    bool isReadEof();
    /// 是否已转码eof, 注意此时不一定readEof, 需要调用read把剩余未读取的数据读取完;
    bool isTranscodingEof();
    
    /// 获取pkt缓冲的endPts; in output time base;
    int64_t getPacketQueueEndPts();
    /// 获取fifo缓冲的endPts; in output time base;
    int64_t getFifoEndPts();
    /// 获取总时长; in output time base;
    int64_t getDuration();
    int getOutputSampleRate();
    AVSampleFormat getOutputSampleFormat();
    int getOutputChannels();
    
private:
    int recreateFilterGraph();
    int createFilterGraph(AVBufferSrcParameters *_Nonnull buf_src_params, int output_sample_rate, AVSampleFormat output_sample_format, const std::string& output_channel_layout_desc, FilterGraph *_Nullable*_Nonnull out_filter_graph);

private:
    AVRational _in_stream_time_base;
    int _in_stream_index;
    int64_t _pkt_queue_end_pts { AV_NOPTS_VALUE };
    int64_t _duration;
    
    int _output_sample_rate;
    AVSampleFormat _output_sample_format;
    std::string _output_channel_layout_desc;
    int _output_channels;
    int _output_bytes_per_sample;
    bool _output_interleaved;
    
    MediaDecoder *_Nullable _decoder { nullptr };
    AVBufferSrcParameters *_Nullable _buf_src_params { nullptr };
    FilterGraph *_Nullable _filter_graph { nullptr };
    PacketQueue *_Nullable _packet_queue { nullptr };
    AudioFifo *_Nullable _fifo { nullptr };
    
    AVPacket *_Nullable _pkt { nullptr };
    AVFrame *_Nullable _dec_frame { nullptr };
    AVFrame *_Nullable _filt_frame { nullptr };
    
    bool _initialized { false };
    bool _packet_reached_eof { false };
    bool _transcoding_eof { false };
    bool _should_align_frames { false };
    bool _should_drain_packets { false };
};

}

#endif //FFAV_SingleStreamAudioTranscoder_hpp
