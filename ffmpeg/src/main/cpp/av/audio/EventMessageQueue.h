//
// Created on 2025/2/19.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef FFMPEG_HARMONY_OS_EVENTMESSAGEQUEUE_H
#define FFMPEG_HARMONY_OS_EVENTMESSAGEQUEUE_H

#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include "EventMessage.h"

namespace FFAV {

class EventMessageQueue {
public:
    EventMessageQueue();
    ~EventMessageQueue();

    using EventCallback = std::function<void(std::shared_ptr<EventMessage> msg)>;
    void setEventCallback(EventCallback callback);
    void push(std::shared_ptr<EventMessage> msg);
    void clear();

private:
    EventCallback event_callback;
    std::mutex mtx;
    std::condition_variable msg_cv;
    std::unique_ptr<std::thread> msg_thread = nullptr;
    std::queue<std::shared_ptr<EventMessage>> msg_queue;
    bool is_running = true;
    
    void ProcessQueue();
};

}


#endif //FFMPEG_HARMONY_OS_EVENTMESSAGEQUEUE_H
