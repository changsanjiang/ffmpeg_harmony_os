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

#include "PacketQueue.h"
#include <stdint.h>

namespace FFAV {

PacketQueue::PacketQueue() = default;

PacketQueue::~PacketQueue() {
    clear();
}

void PacketQueue::push(AVPacket* _Nonnull packet) {
    AVPacket* pkt = av_packet_alloc();
    av_packet_ref(pkt, packet);
    
    queue.push(pkt);
    total_size += pkt->size;
    last_push_pts = pkt->pts;
}

bool PacketQueue::pop(AVPacket* _Nonnull packet) {
    if ( queue.empty() ) {
        return false;
    }

    AVPacket* pkt = queue.front();
    queue.pop();
    total_size -= pkt->size;
    last_pop_pts = pkt->pts;
    
    av_packet_move_ref(packet, pkt);
    av_packet_free(&pkt);
    return true;
}

void PacketQueue::clear() {
    while(!queue.empty()) {
        AVPacket* pkt = queue.front();
        queue.pop();
        av_packet_free(&pkt);
    }
    total_size = 0;
    last_push_pts = AV_NOPTS_VALUE;
    last_pop_pts = AV_NOPTS_VALUE;
}

int64_t PacketQueue::getLastPushPts() {
    return last_push_pts;
}

int64_t PacketQueue::getLastPopPts() {
    return last_pop_pts;
}

int64_t PacketQueue::getFrontPacketPts() {
    return queue.empty() ? AV_NOPTS_VALUE : queue.front()->pts;
}

size_t PacketQueue::getCount() {
    return queue.size();
}

int64_t PacketQueue::getSize() {
    return total_size;
}

}
