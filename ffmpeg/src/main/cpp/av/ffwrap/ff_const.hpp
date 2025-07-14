//
// Created on 2025/7/3.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef FFMPEG_HARMONY_OS_LAME_FF_CONST_H
#define FFMPEG_HARMONY_OS_LAME_FF_CONST_H

namespace FFAV {
    enum class FlushMode {
        /// 只清除pkt相关的缓存(pkts + decoder + filterGraph);
        PacketOnly,
        /// 清除所有缓存(pkts + decoder + filterGraph + fifo);
        Full,
    };
}

#endif //FFMPEG_HARMONY_OS_LAME_FF_CONST_H
