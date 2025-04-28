//
// Created on 2025/4/28.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef FFMPEG_HARMONY_OS_AUDIOTRANSCODER_H
#define FFMPEG_HARMONY_OS_AUDIOTRANSCODER_H

#include "av/core/AudioFifo.h"
#include "av/core/MediaDecoder.h"
#include "av/core/PacketQueue.h"
#include "av/core/FilterGraph.h"
#include <ohaudio/native_audiostream_base.h>
#include <stdint.h>

namespace FFAV {

/** 音频流转码
 * 
 * 输出格式固定为 44100, 2, s16le
 * */
class AudioTranscoder {
    
public:
    static int OUTPUT_SAMPLE_RATE;
    static AVSampleFormat OUTPUT_SAMPLE_FORMAT;
    static int OUTPUT_CHANNELS;
    static std::string OUTPUT_CHANNEL_LAYOUT_DESC;
    
    static OH_AudioStream_SampleFormat OH_OUTPUT_SAMPLE_FORMAT;
    
    AudioTranscoder();
    ~AudioTranscoder();
    
    int prepareByAudioStream(AVStream *stream);
    /** 添加需要解码的数据包, 传入 nullptr 表示读取已 eof;  */
    int push(AVPacket *pkt);
    int flush(bool shouldOnlyFlushPackets = false);
    
    /**
     * 尝试转码出指定数量的音频数据;
     * 
     * 数据足够时返回值与 frameCapacity 一致;
     * 当 eof 时可能返回的样本数量小于指定的样本数量;
     * 如果未到 eof 数据不满足指定的样本数量时返回 0;
     * 
     * @param outPts   outData pts, in output timeBase; */
    int tryTranscode(int frameCapacity, void **outData, int64_t *outPts, bool *outEOF);
    
    /** 缓冲是否已满; */
    bool isPacketBufferFull();
    
    /** 是否转码完毕; */
    bool isEOF();
    
    /**
     * 可播放到的时间, 一般是最后一个pkt的pts, 当读取 eof 时返回音频流的播放时长;
     * 
     * in stream time base;
     * */
    int64_t getPlayableEndTime();
    
    /**
     * 获取 fifo 中的最后一帧的 pts;
     * */
    int64_t getFifoEndTime();
    
    AVRational getStreamTimeBase();
    AVRational getOutputTimeBase();
    
private:
    int64_t mStreamDuration;
    AVRational mStreamTimeBase;
    
    int mOutputBytesPerSample;
    
    std::string mFilterBufSrcName = "0:a";
    std::string mFilterBufSinkName = "output";
    
    int mPacketSizeThreshold { 5 * 1024 * 1024 }; // bytes; 5M;
    
    AVBufferSrcParameters *mBufferSrcParams { nullptr };
    FFAV::MediaDecoder *mAudioDecoder { nullptr };
    FFAV::FilterGraph *mFilterGraph { nullptr };
    FFAV::PacketQueue *mPacketQueue { nullptr };
    FFAV::AudioFifo *mAudioFifo { nullptr };
    
    AVPacket *mPacket { nullptr };
    AVFrame *mDecFrame { nullptr };
    AVFrame *mFiltFrame { nullptr };
    
    bool mPrepared { false };
    bool mPacketEOF { false };
    bool mTranscodingEOF { false };
    bool mShouldAlignFrames { false };
    
    bool mShouldDrainPackets { false }; // 控制缓冲， 确保流畅播放(满3s)
    
    int recreateFilterGraph();
    int createFilterGraph(FFAV::FilterGraph **outFilterGraph);
};

}

#endif //FFMPEG_HARMONY_OS_AUDIOTRANSCODER_H
