/**
    This file is part of @sj/ffmpeg.
    
    @sj/ffmpeg is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    
    @sj/ffmpeg is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with @sj/ffmpeg. If not, see <http://www.gnu.org/licenses/>.
 * */
//
// Created on 2025/2/11.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef FFMPEGPROJ_PLAYWHENREADYCHANGEREASON_H
#define FFMPEGPROJ_PLAYWHENREADYCHANGEREASON_H
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
}
#endif //FFMPEGPROJ_PLAYWHENREADYCHANGEREASON_H
