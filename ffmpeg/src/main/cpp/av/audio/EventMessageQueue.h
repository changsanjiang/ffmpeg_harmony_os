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
    void stop();

private:
    EventCallback event_callback = nullptr;
    std::mutex mtx;
    std::condition_variable msg_cv;
    std::unique_ptr<std::thread> msg_thread = nullptr;
    std::queue<std::shared_ptr<EventMessage>> msg_queue;
    bool is_running = true;
    
    void ProcessQueue();
};

}


#endif //FFMPEG_HARMONY_OS_EVENTMESSAGEQUEUE_H
