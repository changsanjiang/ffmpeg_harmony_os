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

namespace FFAV {

PacketQueue::PacketQueue() = default;

PacketQueue::~PacketQueue() {
    clear();
}

void PacketQueue::push(AVPacket* _Nonnull packet) {
    std::lock_guard<std::mutex> lock(mtx);
    AVPacket* pkt = av_packet_alloc();
    av_packet_ref(pkt, packet);
    
    queue.push(pkt);
    total_size += pkt->size;
}

bool PacketQueue::pop(AVPacket* _Nonnull packet) {
    std::lock_guard<std::mutex> lock(mtx);
    if ( queue.empty() ) {
        return false;
    }

    AVPacket* pkt = queue.front();
    queue.pop();
    
    av_packet_move_ref(packet, pkt);
    total_size -= pkt->size;
    av_packet_free(&pkt);
    return true;
}

void PacketQueue::clear() {
    std::lock_guard<std::mutex> lock(mtx);
    while(!queue.empty()) {
        AVPacket* pkt = queue.front();
        queue.pop();
        av_packet_free(&pkt);
    }
    total_size = 0;
}

size_t PacketQueue::getCount() {
    std::lock_guard<std::mutex> lock(mtx);
    return queue.size();
}

int64_t PacketQueue::getSize() {
    std::lock_guard<std::mutex> lock(mtx);
    return total_size;
}

}