//
// Created on 2025/7/4.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef FFMPEG_HARMONY_OS_LAME_FF_STREAM_PROVIDER_H
#define FFMPEG_HARMONY_OS_LAME_FF_STREAM_PROVIDER_H

#include "ff_types.hpp"

namespace FFAV {

/** 提供流信息 */
class StreamProvider {
public:
    StreamProvider() = default;
    ~StreamProvider() = default;
    
    StreamProvider(StreamProvider&&) noexcept = delete;
    StreamProvider& operator=(StreamProvider&&) noexcept = delete;

    // 获取流的数量
    virtual unsigned int getStreamCount() = 0;
    virtual unsigned int getStreamCount(AVMediaType type) = 0;

    // 获取指定流的 AVStream
    virtual AVStream* getStream(int stream_index) = 0;    
    virtual AVStream* getBestStream(AVMediaType type) = 0;
    virtual AVStream* getFirstStream(AVMediaType type) = 0;
    virtual AVStream** getStreams() = 0;
};

}

#endif //FFMPEG_HARMONY_OS_LAME_FF_STREAM_PROVIDER_H
