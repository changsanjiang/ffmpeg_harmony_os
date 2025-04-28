//
// Created on 2025/4/28.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "AudioTranscoder.h"
#include <stdint.h>
#include <sstream>
#include "av/core/AudioUtils.h"

namespace FFAV {

int AudioTranscoder::OUTPUT_SAMPLE_RATE = 44100;
AVSampleFormat AudioTranscoder::OUTPUT_SAMPLE_FORMAT = AV_SAMPLE_FMT_S16;
int AudioTranscoder::OUTPUT_CHANNELS = 2;
std::string AudioTranscoder::OUTPUT_CHANNEL_LAYOUT_DESC = "stereo";

OH_AudioStream_SampleFormat AudioTranscoder::OH_OUTPUT_SAMPLE_FORMAT = OH_AudioStream_SampleFormat::AUDIOSTREAM_SAMPLE_S16LE;

AudioTranscoder::AudioTranscoder() {
    
}

AudioTranscoder::~AudioTranscoder() {
    if ( mAudioDecoder ) {
        delete mAudioDecoder;
    }
    
    if ( mBufferSrcParams ) {
        av_free(mBufferSrcParams);
    }
    
    if ( mFilterGraph ) {
        delete mFilterGraph;
    }
    
    if ( mPacketQueue ) {
        delete mPacketQueue;
    }
    
    if ( mAudioFifo ) {
        delete mAudioFifo;
    }
    
    if ( mPacket ) {
        av_packet_free(&mPacket);
    }
    
    if ( mDecFrame ) {
        av_frame_free(&mDecFrame);
    }
    
    if ( mFiltFrame ) {
        av_frame_free(&mFiltFrame);
    }
}

int AudioTranscoder::prepareByAudioStream(AVStream *stream) {
    if ( mPrepared ) {
        throw std::runtime_error("AudioTranscoder is already prepared");
    }
    
    int ff_ret = 0;
    if ( stream->codecpar == nullptr ) {
        ff_ret = AVERROR_DECODER_NOT_FOUND;
        goto on_exit; // exit;
    }
    
    mStreamDuration = stream->duration;
    mStreamTimeBase = stream->time_base;
    
    mOutputBytesPerSample = av_get_bytes_per_sample(OUTPUT_SAMPLE_FORMAT);

    // init decoder
    mAudioDecoder = new FFAV::MediaDecoder();
    ff_ret = mAudioDecoder->init(stream->codecpar);
    if ( ff_ret < 0 ) {
        goto on_exit; // exit;
    }
    
    // create buffer src params
    mBufferSrcParams = mAudioDecoder->createBufferSrcParameters(stream->time_base);
    
    // init filter graph
    ff_ret = createFilterGraph(&mFilterGraph);
    if ( ff_ret < 0 ) {
        goto on_exit; // exit;
    }
    
    // init pkt queue
    mPacketQueue = new FFAV::PacketQueue();
    
    // init audio fifo
    mAudioFifo = new FFAV::AudioFifo();
    ff_ret = mAudioFifo->init(OUTPUT_SAMPLE_FORMAT, OUTPUT_CHANNELS, 1);
    if ( ff_ret < 0 ) {
        goto on_exit; // exit;
    }
    
    // ready
    mPacket = av_packet_alloc();
    mDecFrame = av_frame_alloc();
    mFiltFrame = av_frame_alloc();

    mPrepared = true;
on_exit:
    return ff_ret;
}

int AudioTranscoder::push(AVPacket *pkt) {
    if ( pkt ) {
        mPacketQueue->push(pkt);
    }
    else {
        mPacketEOF = true;
        
        if ( mPacketQueue->getCount() == 0 ) {
            mTranscodingEOF = true;
        }
    }
    return 0;
}

int AudioTranscoder::flush(bool shouldOnlyFlushPackets) {
    if ( !shouldOnlyFlushPackets ) {
        mPacketEOF = false;
        mTranscodingEOF = false;
        
        mAudioFifo->clear();
        mPacketQueue->clear();
        mAudioDecoder->flush();
     
        int ret = 0;
        ret = recreateFilterGraph();
        if ( ret < 0 ) {
            return ret;
        }
    }
    else {
        mPacketEOF = false;
        mTranscodingEOF = false;
        mShouldAlignFrames = mAudioFifo->getNumberOfSamples() > 0;
    
        mPacketQueue->clear();
        mAudioDecoder->flush();
     
        int ret = 0;
        ret = recreateFilterGraph();
        if ( ret < 0 ) {
            return ret;
        }
    }
    return 0;
}

int AudioTranscoder::tryTranscode(int frameCapacity, void **outData, int64_t *outPts, bool *outEOF) {
    if ( !mPrepared ) {
        return 0;
    }
    
    // 控制缓冲， 确保流畅播放(满3s)
    if ( !mShouldDrainPackets ) {
        if ( mPacketEOF ) {
            mShouldDrainPackets = true;
        }
        else {
            int64_t startPts = mPacketQueue->getFrontPacketPts();
            int64_t endPts = mPacketQueue->getLastPushPts();
            if ( endPts != AV_NOPTS_VALUE && startPts != AV_NOPTS_VALUE ) {
                if ( endPts - startPts >= av_rescale_q(3, (AVRational){ 1, 1 }, mStreamTimeBase) ) {
                    mShouldDrainPackets = true; // 需要榨干pkts
                }
            }
        }
    }
    
    if ( !mShouldDrainPackets ) {
        return 0;
    }
    
    // transcoding
    if ( !mTranscodingEOF ) {
        int ff_ret = 0;
        do {
            // 如果转码后的数据足够或者已转码结束, 则退出循环
            if ( mAudioFifo->getNumberOfSamples() >= frameCapacity || mTranscodingEOF ) {
                break;
            }
            
            // 当前无可转码数据时, 退出循环
            if ( !mPacketEOF && mPacketQueue->getSize() == 0 ) {
                break;
            }
            
            // 有可转码数据或已`read eof`
            AVPacket* nextPacket = nullptr;
            if ( mPacketQueue->pop(mPacket) ) {
                nextPacket = mPacket;
            }
            
            // do transcode
            ff_ret = FFAV::AudioUtils::transcode(nextPacket, mAudioDecoder, mDecFrame, mFilterGraph, mFiltFrame, mFilterBufSrcName, mFilterBufSinkName, [&](AVFrame *filtFrame) {
                // flush packets 已完成 && 需要对齐时
                if ( mShouldAlignFrames ) {
                    int64_t aligned_pts = mAudioFifo->getEndPts();
                    int64_t start_pts = filtFrame->pts;
                    if ( aligned_pts != AV_NOPTS_VALUE && aligned_pts != start_pts ) {
                        if ( start_pts > aligned_pts ) {
                            return AVERROR_BUG2;
                        }

                        int64_t end_pts = start_pts + filtFrame->nb_samples;
                        if ( aligned_pts >= end_pts ) {
                            return 0;
                        }

                        // intersecting samples
                        int64_t nb_samples = end_pts - aligned_pts;
                        if ( av_sample_fmt_is_planar(OUTPUT_SAMPLE_FORMAT) ) {
                            int64_t pos_offset = (aligned_pts - start_pts) * mOutputBytesPerSample;
                            void* data[OUTPUT_CHANNELS];
                            for (int i = 0; i < OUTPUT_CHANNELS; ++i) {
                                data[i] = filtFrame->data[i] + pos_offset;
                            }
                            mShouldAlignFrames = false;
                            return mAudioFifo->write(data, (int)nb_samples, aligned_pts);
                        }
                        else {
                            int64_t pos_offset = (aligned_pts - start_pts) * mOutputBytesPerSample * OUTPUT_CHANNELS;
                            void* data = filtFrame->data[0] + pos_offset;
                            mShouldAlignFrames = false;
                            return mAudioFifo->write(&data, (int)nb_samples, aligned_pts);
                        }
                    }
                    else {
                        mShouldAlignFrames = false;
                    }
                }
                return mAudioFifo->write((void **)filtFrame->data, filtFrame->nb_samples, filtFrame->pts);
            });
            
            if ( nextPacket ) {
                av_packet_unref(nextPacket);
            }
            
            // eof
            if ( ff_ret == AVERROR_EOF ) {
                mTranscodingEOF = true;
                break;
            }
            else if ( ff_ret == AVERROR(EAGAIN) ) {
                // nothing
            }
            // error
            else if ( ff_ret < 0 ) {
                break;
            }
        } while (true);
        
        // transcode error
        if ( ff_ret < 0 && ff_ret != AVERROR_EOF && ff_ret != AVERROR(EAGAIN) ) {
            return ff_ret;
        }
    }
    
    // read data
    int ret = 0;
    if ( mAudioFifo->getNumberOfSamples() > 0 ) {
        if ( mAudioFifo->getNumberOfSamples() >= frameCapacity || mTranscodingEOF ) {
            int64_t pts = 0;
            ret = mAudioFifo->read(outData, frameCapacity, &pts);
            if ( outPts ) *outPts = pts;
        }
    }
    
    // set eof
    if ( outEOF ) {
        *outEOF = mTranscodingEOF && mAudioFifo->getNumberOfSamples() == 0;
    }
    
    // 已榨干pkts
    if ( mShouldDrainPackets && mPacketQueue->getCount() == 0 ) {
        mShouldDrainPackets = false;
    }
    return ret;
}

bool AudioTranscoder::isPacketBufferFull() {
    return mPacketQueue->getSize() >= mPacketSizeThreshold;
}

bool AudioTranscoder::isEOF() {
    return mTranscodingEOF && mAudioFifo->getNumberOfSamples() == 0;
}

int64_t AudioTranscoder::getPlayableEndTime() {
    if ( mPacketEOF ) {
        return mStreamDuration;
    }
    int64_t lastPushPts = mPacketQueue->getLastPushPts();   // range end
    return lastPushPts != AV_NOPTS_VALUE ? lastPushPts : 0;
}

int64_t AudioTranscoder::getFifoEndTime() {
    int64_t endPts = mAudioFifo->getEndPts(); 
    return endPts != AV_NOPTS_VALUE ? endPts : 0;
}

AVRational AudioTranscoder::getStreamTimeBase() {
    return mStreamTimeBase;
}

AVRational AudioTranscoder::getOutputTimeBase() {
    return (AVRational){ 1, OUTPUT_SAMPLE_RATE };
}

int AudioTranscoder::recreateFilterGraph() {
    if ( mFilterGraph ) {
        delete mFilterGraph;
        mFilterGraph = nullptr;
    }
    return createFilterGraph(&mFilterGraph);
}

int AudioTranscoder::createFilterGraph(FFAV::FilterGraph **outFilterGraph) {
    FFAV::FilterGraph *filterGraph = new FFAV::FilterGraph();
    std::stringstream filterDescr;
    int ret = 0;
    
    ret = filterGraph->init();
    if ( ret < 0 ) {
        goto on_exit;
    }
    
    if ( ret < 0 ) {
        goto on_exit;
    }
    
    ret = filterGraph->addBufferSourceFilter(mFilterBufSrcName, AVMEDIA_TYPE_AUDIO, mBufferSrcParams);
    if ( ret < 0 ) {
        goto on_exit;
    }
    
    ret = filterGraph->addAudioBufferSinkFilter(mFilterBufSinkName, OUTPUT_SAMPLE_RATE, OUTPUT_SAMPLE_FORMAT, OUTPUT_CHANNEL_LAYOUT_DESC);
    if ( ret < 0 ) {
        goto on_exit;
    }
    
    filterDescr << "[" << mFilterBufSrcName << "]"
                    << "aformat=sample_fmts=" << av_get_sample_fmt_name(OUTPUT_SAMPLE_FORMAT) << ":channel_layouts=" << OUTPUT_CHANNEL_LAYOUT_DESC << ",aresample=" << OUTPUT_SAMPLE_RATE
                    << "[" << mFilterBufSinkName << "]";

    ret = filterGraph->parse(filterDescr.str());
    
    if ( ret < 0 ) {
        goto on_exit;
    }
    
    ret = filterGraph->configure();
    if ( ret < 0 ) {
        goto on_exit;
    }
    
on_exit:
    if ( ret == 0 ) {
        *outFilterGraph = filterGraph;
    }
    return ret;
}

}