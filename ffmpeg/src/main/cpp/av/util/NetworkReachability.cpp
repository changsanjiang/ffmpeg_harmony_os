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

#include "NetworkReachability.h"
#include "network/netmanager/net_connection.h"
#include "network/netmanager/net_connection_type.h"

namespace FFAV {

void NetworkReachability::OnNetworkAvailable(NetConn_NetHandle *netHandle) {
    NetworkReachability::shared().setStatus(NetworkStatus::AVAILABLE);
}

void NetworkReachability::OnNetLost(NetConn_NetHandle *netHandle) {
    NetworkReachability::shared().setStatus(NetworkStatus::LOST);
}

void NetworkReachability::OnNetUnavailable() {
    NetworkReachability::shared().setStatus(NetworkStatus::UNAVAILABLE);
}

NetworkReachability& NetworkReachability::shared() {
    static NetworkReachability instance;
    return instance;
}

NetworkReachability::NetworkReachability() {
    NetConn_NetConnCallback netConnCallback = { nullptr };
    netConnCallback.onNetworkAvailable = OnNetworkAvailable;
    netConnCallback.onNetUnavailable = OnNetUnavailable;
    netConnCallback.onNetLost = OnNetLost;
    OH_NetConn_RegisterDefaultNetConnCallback(&netConnCallback, &oh_callback_id);
}

NetworkReachability::~NetworkReachability() {
    OH_NetConn_UnregisterNetConnCallback(oh_callback_id);
}

NetworkStatus NetworkReachability::getStatus() {
    return status.load();   
}

uint32_t NetworkReachability::addNetworkStatusChangeCallback(NetworkStatusChangeCallback callback) {
    std::lock_guard<std::mutex> lock(mtx);
    int callback_id = next_callback_id;
    next_callback_id += 1;
    callbacks[callback_id] = callback;
    return callback_id;
}

void NetworkReachability::removeNetworkStatusChangeCallback(uint32_t callback_id) {
    std::lock_guard<std::mutex> lock(mtx);
    callbacks.erase(callback_id);
}

void NetworkReachability::setStatus(NetworkStatus status) {
    this->status.store(status);
    std::lock_guard<std::mutex> lock(mtx);
    for (const auto& pair : callbacks) {
        pair.second(status);
    }
}

}