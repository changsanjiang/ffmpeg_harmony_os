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
// Created on 2025/3/3.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef FFMPEG_HARMONY_OS_NETWORKREACHABILITY_H
#define FFMPEG_HARMONY_OS_NETWORKREACHABILITY_H

#include "stdint.h"
#include <atomic>
#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <network/netmanager/net_connection_type.h>

namespace FFAV {

enum NetworkStatus {
    UNAVAILABLE,
    AVAILABLE,
    LOST
};

class NetworkReachability {
public:
    static NetworkReachability& shared();
    NetworkStatus getStatus();
    
    using NetworkStatusChangeCallback = std::function<void(NetworkStatus status)>;
    uint32_t addNetworkStatusChangeCallback(NetworkStatusChangeCallback callback);
    void removeNetworkStatusChangeCallback(uint32_t callback_id);
    
private:
    static void OnNetworkAvailable(NetConn_NetHandle *netHandle);
    static void OnNetLost(NetConn_NetHandle *netHandle);
    static void OnNetUnavailable();
    
    NetworkReachability();
    ~NetworkReachability();

    uint32_t oh_callback_id;
    std::atomic<NetworkStatus> status { NetworkStatus::UNAVAILABLE };
    std::mutex mtx;
    std::map<uint32_t, NetworkStatusChangeCallback> callbacks;
    uint32_t next_callback_id { 0 };
    
    void setStatus(NetworkStatus status);
};

}

#endif //FFMPEG_HARMONY_OS_NETWORKREACHABILITY_H
