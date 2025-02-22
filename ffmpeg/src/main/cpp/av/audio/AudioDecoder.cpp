//
// Created on 2025/2/22.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "AudioDecoder.h"
#include <memory>

namespace FFAV {
    
AudioDecoder::AudioDecoder(const std::string &url, int64_t start_time_position_ms): url(url), start_time_position_ms(start_time_position_ms) {
    
}

AudioDecoder::~AudioDecoder() {
    
}

void AudioDecoder::prepare() {
    std::lock_guard<std::mutex> lock(mtx);
    if ( !read_thread ) {
        read_thread = std::make_unique<std::thread>(&AudioDecoder::ReadThread, this);
    }
}

void AudioDecoder::ReadThread() {
    // - 进行初始化
    // - 读取数据包
    
    std::lock_guard<std::mutex> lock(mtx);
    if ( flags.release_invoked ) {
        return;
    }
    
    init();
}

void AudioDecoder::DecThread() {
    // 解码数据包
    
}


void AudioDecoder::init() {
    
}

}