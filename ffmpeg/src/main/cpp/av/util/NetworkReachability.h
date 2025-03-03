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
    
    void setStatus(NetworkStatus status);
private:
    NetworkReachability();
    ~NetworkReachability();

    uint32_t oh_callback_id;
    std::atomic<NetworkStatus> status { NetworkStatus::UNAVAILABLE };
    std::mutex mtx;
    std::map<uint32_t, NetworkStatusChangeCallback> callbacks;
    uint32_t next_callback_id { 0 };
};

}

#endif //FFMPEG_HARMONY_OS_NETWORKREACHABILITY_H
