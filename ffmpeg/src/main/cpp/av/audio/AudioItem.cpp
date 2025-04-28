//
// Created on 2025/4/28.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "AudioItem.h"
#include <stdint.h>

namespace FFAV {

AudioItem::AudioItem(const std::string &url, int64_t start_time_pos_ms, const std::map<std::string, std::string> &http_options): mUrl(url), mHttpOptions(http_options) {
    mReader = new AudioReader(url, http_options);
    mTranscoder = new AudioTranscoder();
    
    if ( start_time_pos_ms > 0 ) {
        mStartTimePosMs = start_time_pos_ms;
    }
}

AudioItem::~AudioItem() {
    {
        std::unique_lock<std::mutex> lock(mtx);
        mFlags.mReleased = true;
    }
    
    if ( mRepreareReaderTask ) {
        mRepreareReaderTask->tryCancel();
        mRepreareReaderTask = nullptr;
    }
    
    if ( mFlags.mRegisteredNetworkStatusChangeCallback ) {
        NetworkReachability::shared().removeNetworkStatusChangeCallback(mNetworkStatusChangeCallbackId);
    }
    
    mReader->stop();
    delete mReader;
    mReader = nullptr;
    
    delete mTranscoder;
    mTranscoder = nullptr;
}

int AudioItem::getOutputSampleRate() {
    return AudioTranscoder::OUTPUT_SAMPLE_RATE;
}

AVSampleFormat AudioItem::getOutputSampleFormat() {
    return AudioTranscoder::OUTPUT_SAMPLE_FORMAT;
}

OH_AudioStream_SampleFormat AudioItem::getOutputOHSampleFormat() {
    return AudioTranscoder::OH_OUTPUT_SAMPLE_FORMAT;
}

int AudioItem::getOutputChannels() {
    return AudioTranscoder::OUTPUT_CHANNELS;
}

void AudioItem::prepare() {
    std::unique_lock<std::mutex> lock(mtx);
    if ( mFlags.mPrepareInvoked ) {
        return;
    }
    mFlags.mPrepareInvoked = true;
    mReader->setReadyToReadCallback(std::bind(&AudioItem::onReadyToRead, this, std::placeholders::_1, std::placeholders::_2));
    mReader->setReadPacketCallback(std::bind(&AudioItem::onReadPacket, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    mReader->setReadErrorCallback(std::bind(&AudioItem::onReadError, this, std::placeholders::_1, std::placeholders::_2));
    mReader->prepare(mStartTimePosMs);
}

int AudioItem::tryTranscode(int frameCapacity, void **outData, int64_t *outPtsMs, bool *outEOF) {
    std::lock_guard<std::mutex> lock(mtx);
    if ( !mFlags.mReady || mFlags.mSeeking ) {
        return 0;
    }
    
    int64_t pts = 0;
    int ret = mTranscoder->tryTranscode(frameCapacity, outData, &pts, outEOF);
    if ( ret > 0 ) {
        *outPtsMs = av_rescale_q(pts, mTranscoder->getOutputTimeBase(), (AVRational){ 1, 1000 });
        
        if ( !mTranscoder->isPacketBufferFull() ) {
            mReader->setPacketBufferFull(false);
        }
    }
    return ret;
}

void AudioItem::seek(int64_t timeMs) {
    std::lock_guard<std::mutex> lock(mtx);
    mFlags.mSeeking = true;
    
    if ( mFlags.mShouldReprepareReader ) {
        mFlags.mSeekedBeforeReprepareReader = true;
        mFlags.mShouldOnlyFlushPackets = false;
        mStartTimePosMs = timeMs;
        onReprepareReader(mStartTimePosMs);
        return;
    }

    mReader->seek(timeMs);
}

int64_t AudioItem::getDurationMs() {
    return mDurationMs.load(std::__n1::memory_order_relaxed);
}

int64_t AudioItem::getPlayableDurationMs() {
    return mPlayableDurationMs.load(std::__n1::memory_order_relaxed);
}

void AudioItem::setOnDurationChangeCallback(AudioItem::OnDurationChangeCallback callback) {
    mOnDurationChangeCallback = callback;
}

void AudioItem::setOnErrorCallback(AudioItem::OnErrorCallback callback) {
    mOnErrorCallback = callback;
}

void AudioItem::setOnPlayableDurationChangeCallback(AudioItem::OnPlayableDurationChangeCallback callback) {
    mOnPlayableDurationChangeCallback = callback;
}

void AudioItem::onReadyToRead(AudioReader* reader, AVStream* stream) {
    int ff_ret = 0;
    int64_t durationMs = 0;
    {
        std::unique_lock<std::mutex> lock(mtx);
        if ( mFlags.mReleased ) {
            return; // return directly;
        }
        
        // reprepare
        if ( mFlags.mReady ) {
            reader->start();
            return; // return directly;
        }
        
        durationMs = av_rescale_q(stream->duration, stream->time_base, (AVRational){ 1, 1000 });
        mDurationMs.store(durationMs, std::__n1::memory_order_relaxed);
        ff_ret = mTranscoder->prepareByAudioStream(stream);
        
        // ready
        if ( ff_ret == 0 ) {
            mFlags.mReady = true;
            reader->start();
        }
    }
    
    if ( ff_ret < 0 ) {
        if ( mOnErrorCallback ) mOnErrorCallback(ff_ret);
        return;
    }
    
    if ( mOnDurationChangeCallback ) mOnDurationChangeCallback(durationMs);
}

void AudioItem::onReadPacket(AudioReader* reader, AVPacket* pkt, bool should_flush) {
    int ff_ret = 0;
    int64_t playableDurationMs = AV_NOPTS_VALUE;
    {
        std::lock_guard<std::mutex> lock(mtx);
        if ( mFlags.mReleased ) {
            return; // return directly;
        }

        if ( should_flush ) {
            mFlags.mSeeking = false;
            mFlags.mSeekedBeforeReprepareReader = false;
        }
        
        // return if seeking
        if ( mFlags.mSeeking ) {
            goto on_exit;
        }
    
        // flush
        if ( should_flush ) {
            ff_ret = mTranscoder->flush(mFlags.mShouldOnlyFlushPackets);
            if ( ff_ret < 0 ) {
                goto on_exit;
            }
            mFlags.mShouldOnlyFlushPackets = false;
        }
        
        // push pkt
        ff_ret = mTranscoder->push(pkt);
         if ( ff_ret < 0 ) {
            goto on_exit;
        }
        
        if ( mTranscoder->isPacketBufferFull() ) {
            reader->setPacketBufferFull(true);
        }
        
        playableDurationMs = av_rescale_q(mTranscoder->getPlayableEndTime(), mTranscoder->getStreamTimeBase(), (AVRational){ 1, 1000 });
        mPlayableDurationMs.store(playableDurationMs, std::__n1::memory_order_relaxed);
    }
on_exit:
    
    if ( ff_ret < 0 ) {
        if ( mOnErrorCallback ) mOnErrorCallback(ff_ret);
        return;
    }
    
    if ( playableDurationMs != AV_NOPTS_VALUE ) {
        if ( mOnPlayableDurationChangeCallback ) mOnPlayableDurationChangeCallback(playableDurationMs);
    }
}

void AudioItem::onReadError(AudioReader* reader, int ff_err) {
    {
        std::lock_guard<std::mutex> lock(mtx);
        if ( ff_err == AVERROR(EIO) ||
             ff_err == AVERROR(ENETDOWN) ||
             ff_err == AVERROR(ENETUNREACH) ||
             ff_err == AVERROR(ENETRESET) ||
             ff_err == AVERROR(ECONNABORTED) ||
             ff_err == AVERROR(ECONNRESET) ||
             ff_err == AVERROR(ETIMEDOUT) ||
             ff_err == AVERROR(EHOSTUNREACH) ||
             ff_err == AVERROR_HTTP_SERVER_ERROR ||
             (ff_err == AVERROR_INVALIDDATA && mFlags.mReady)
            ) {
            // 遇到可恢复的错误时重置reader并重新准备
            setNeedsReprepareReader();
            return;
        }
        
        reader->stop();
    }
    
    if ( mOnErrorCallback ) mOnErrorCallback(ff_err);
}
            
void AudioItem::setNeedsReprepareReader() {
    mFlags.mShouldReprepareReader = true;
            
    NetworkStatus network_status = NetworkReachability::shared().getStatus();
    // 无网络的时候监听网络状态变更;
    // 可用时重新初始化 reader;
    if ( !mFlags.mRegisteredNetworkStatusChangeCallback ) {
        if ( network_status != NetworkStatus::AVAILABLE ) {
            mFlags.mRegisteredNetworkStatusChangeCallback = true;
            mNetworkStatusChangeCallbackId = NetworkReachability::shared().addNetworkStatusChangeCallback(std::bind(&AudioItem::onNetworkStatusChange, this, std::placeholders::_1));
            return;
        }
    }

    // 有网络时延迟n秒后重试;
    if ( network_status == NetworkStatus::AVAILABLE ) {
        mRepreareReaderTask = TaskScheduler::scheduleTask([&] {
            reprepareReaderIfNeeded();
        }, 2);
    }
}

void AudioItem::reprepareReaderIfNeeded() {
    std::lock_guard<std::mutex> lock(mtx);
    if ( mFlags.mShouldReprepareReader ) {
        // 遇到可恢复的错误时重新准备 reader
        // 例如网络不通时延迟n秒后重试
        // 如果之前未完成初始化, 则直接重新创建 reader 即可;
        // 如果已完成初始化, 则需要考虑读取的开始位置;
        // - 等待期间用户可能调用seek, 需要从seek的位置开始播放(模糊位置)
        // - 如果未执行seek操作, 则需要从当前位置开始播放(精确位置), 需要在解码时对齐数据
        if ( !mFlags.mReady || mFlags.mSeekedBeforeReprepareReader ) {
            onReprepareReader(mStartTimePosMs);
            return;
        }
        
        // - 如果未执行seek操作, 则需要从当前位置开始播放(精确位置), 需要在解码时对齐数据
        int64_t endPts = mTranscoder->getFifoEndTime();
        // 需要保留 fifo 中的缓存, 并且新的数据需要对齐到 fifo 中;
        mFlags.mShouldOnlyFlushPackets = true;
        onReprepareReader(av_rescale_q(endPts, mTranscoder->getOutputTimeBase(), (AVRational){ 1, 1000 }));
    }
}

void AudioItem::onReprepareReader(int64_t startTimePosMs) {
    if ( mRepreareReaderTask ) {
        mRepreareReaderTask->tryCancel();
        mRepreareReaderTask = nullptr;
    }
    mFlags.mShouldReprepareReader = false;
    mReader->reset();
    mReader->prepare(startTimePosMs);
}

void AudioItem::onNetworkStatusChange(NetworkStatus network_status) {
    if ( network_status == NetworkStatus::AVAILABLE ) {
        reprepareReaderIfNeeded();
    }
}

}