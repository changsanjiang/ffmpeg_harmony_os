//
// Created on 2025/4/28.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef FFMPEG_HARMONY_OS_AUDIOITEM_H
#define FFMPEG_HARMONY_OS_AUDIOITEM_H

#include <ohaudio/native_audiostream_base.h>
#include <stdint.h>
#include <functional>
#include <string>
#include <map>
#include <mutex>
#include "av/core/ff_types.hpp"
#include "av/util/NetworkReachability.h"
#include "av/util/TaskScheduler.h"

namespace FFAV {

class AudioPacketReader;
class AudioTranscoder;

class AudioItem {
    
public:
    AudioItem(const std::string& url, int64_t start_time_pos_ms, const std::map<std::string, std::string>& http_options);
    ~AudioItem();
    
    int getOutputSampleRate();
    AVSampleFormat getOutputSampleFormat();
    OH_AudioStream_SampleFormat getOutputOHSampleFormat();
    int getOutputChannels();
    
    void prepare();
    
    /**
     * 尝试转码出指定数量的音频数据;
     * 
     * 数据足够时返回值与 frameCapacity 一致;
     * 当 eof 时可能返回的样本数量小于指定的样本数量;
     * 如果未到 eof 数据不满足指定的样本数量时返回 0;
     *  */
    int tryTranscode(void **outData, int frameCapacity, int64_t *outPtsMs, bool *outEOF);

    void seek(int64_t timeMs);
    
    /** 获取播放时长, 单位毫秒; */
    int64_t getDurationMs();
    
    /** 获取缓冲时长, 单位毫秒; */
    int64_t getPlayableDurationMs();
    
    using OnDurationChangeCallback = std::function<void(int64_t durationMs)>;
    void setOnDurationChangeCallback(OnDurationChangeCallback callback);
    
    using OnErrorCallback = std::function<void(int error)>;
    void setOnErrorCallback(OnErrorCallback callback);
    
    using OnPlayableDurationChangeCallback = std::function<void(int64_t playableDurationMs)>;
    void setOnPlayableDurationChangeCallback(OnPlayableDurationChangeCallback callback);
    
private:
    std::string mUrl;
    int64_t mStartTimePos { AV_NOPTS_VALUE }; // in base q
    std::map<std::string, std::string> mHttpOptions;
    
    AudioPacketReader *mReader { nullptr };
    AudioTranscoder *mTranscoder { nullptr };
    
    std::atomic<int64_t> mDurationMs { 0 };
    std::atomic<int64_t> mPlayableDurationMs { 0 };
    
    std::mutex mtx;
    
    OnDurationChangeCallback mOnDurationChangeCallback { nullptr };
    OnErrorCallback mOnErrorCallback { nullptr };
    OnPlayableDurationChangeCallback mOnPlayableDurationChangeCallback { nullptr };
    
    std::shared_ptr<TaskScheduler> mRepreareReaderTask { nullptr };
    
    uint32_t mNetworkStatusChangeCallbackId { 0 };
    
    struct {
        unsigned int mPrepareInvoked;
        unsigned int mReleased;
        unsigned int mReady;
        
        unsigned int mShouldReprepareReader;
        unsigned int mSeekedBeforeReprepareReader;
        unsigned int mShouldOnlyFlushPackets;
        
        unsigned int mSeeking;
        
        unsigned int mRegisteredNetworkStatusChangeCallback;
    } mFlags { 0 };
    
    void onStreamReady(AudioPacketReader* reader, AVStream* stream);
    void onReadPacket(AudioPacketReader* reader, AVPacket* pkt, bool should_flush);
    void onReadError(AudioPacketReader* reader, int ff_err);
    
    void setNeedsReprepareReader();
    void reprepareReaderIfNeeded();
    void onReprepareReader(int64_t startTimePos); // startTimePos in base q
    
    void onNetworkStatusChange(NetworkStatus status);
};

}

#endif //FFMPEG_HARMONY_OS_AUDIOITEM_H
