//
// Created on 2025/7/4.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef FFAV_AudioConst_hpp
#define FFAV_AudioConst_hpp

#include "av/ffwrap/ff_types.hpp"
#include <map>
#include <ohaudio/native_audiostream_base.h>
#include <stdint.h>

namespace FFAV {

enum PlayWhenReadyChangeReason {
    USER_REQUEST,
    // 恢复播放
    // 此分支表示临时失去焦点后被暂停的音频流此时可以继续播放，建议应用继续播放，切换至音频播放状态
    // 若应用此时不想继续播放，可以忽略此音频焦点事件，不进行处理即可
    // 继续播放，此处主动执行start()，以标识符变量started记录start()的执行结果
    AUDIO_INTERRUPT_RESUME,
    // 此分支表示系统已将音频流暂停（临时失去焦点），为保持状态一致，应用需切换至音频暂停状态
    // 临时失去焦点：待其他音频流释放音频焦点后，本音频流会收到resume对应的音频焦点事件，到时可自行继续播放
    AUDIO_INTERRUPT_PAUSE,
    // 此分支表示系统已将音频流停止（永久失去焦点），为保持状态一致，应用需切换至音频暂停状态
    // 永久失去焦点：后续不会再收到任何音频焦点事件，若想恢复播放，需要用户主动触发。
    AUDIO_INTERRUPT_STOP,
    OLD_DEVICE_UNAVAILABLE,
    PLAYBACK_ENDED,
};

struct AudioPlaybackOptions {
    int64_t start_time_position_ms;
    std::map<std::string, std::string> http_options;
    OH_AudioStream_Usage stream_usage;
};

} // namespace FFAV

#endif //FFAV_AudioConst_hpp
