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
// Created on 2025/2/8.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef FFMPEGPROJ_PACKETQUEUE_H
#define FFMPEGPROJ_PACKETQUEUE_H

#include <cstdint>
extern "C" {
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
}

#include <queue>

namespace FFAV {

class PacketQueue {
public:
    PacketQueue();
    ~PacketQueue();
    
    void push(AVPacket* _Nonnull packet);
    bool pop(AVPacket* _Nonnull packet);
    void clear();
    
    int64_t getLastPushPts();
    int64_t getLastPopPts();

    // 获取所有数据包的数量
    size_t getCount();

    // 获取所有数据包的数据占用的字节数
    int64_t getSize();

private:
    std::queue<AVPacket*> queue;
    int64_t total_size = 0;
    int64_t last_push_pts = AV_NOPTS_VALUE; 
    int64_t last_pop_pts = AV_NOPTS_VALUE;
};

}
#endif //FFMPEGPROJ_PACKETQUEUE_H
