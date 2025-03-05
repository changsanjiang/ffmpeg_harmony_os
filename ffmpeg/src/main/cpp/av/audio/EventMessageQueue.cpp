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

#include "EventMessageQueue.h"

namespace FFAV {

EventMessageQueue::EventMessageQueue() = default;

EventMessageQueue::~EventMessageQueue() {
    stop();
}

void EventMessageQueue::setEventCallback(EventCallback callback) {
    std::unique_lock<std::mutex> lock(mtx);  
    event_callback = callback;
    
    if ( callback ) {
        if ( !msg_thread ) {
            msg_thread = std::make_unique<std::thread>(&EventMessageQueue::ProcessQueue, this);
        }
    }
}

void EventMessageQueue::push(std::shared_ptr<EventMessage> msg) {
    // 这里上锁，确保线程安全
    std::unique_lock<std::mutex> lock(mtx);  
    if ( !is_running || event_callback == nullptr ) {
        return;
    }
    
    msg_queue.push(msg);
    lock.unlock();
    msg_cv.notify_all();
}

void EventMessageQueue::stop() {
    std::unique_lock<std::mutex> lock(mtx);  
    if ( !is_running ) {
        return;
    }
    
    is_running = false;
    
    while (!msg_queue.empty()) {
        msg_queue.pop();
    }
    
    lock.unlock();
    msg_cv.notify_all();
    
    if ( msg_thread && msg_thread->joinable() ) {
        msg_thread->join();
    }
}

void EventMessageQueue::ProcessQueue() {
    while (true) {
        // 这里上锁，确保线程安全
        std::unique_lock<std::mutex> lock(mtx);
        msg_cv.wait(lock, [&] {
            // 需要退出线程了
            if ( !is_running ) {
                return true;
            }
            if ( !event_callback ) {
                return false;
            }
            // 等待消息
            return !msg_queue.empty();
        });
        
        // 需要退出线程, 剩余消息全部丢弃
        if ( !is_running ) {
            return;
        }
        
        // 执行到这里msg和callback必定有值
        // 消费消息
        std::shared_ptr<EventMessage> msg = msg_queue.front();
        auto callback = event_callback;
        msg_queue.pop();
        
        // 提前解锁并进行回调
        lock.unlock();
        
        // 进行回调, 不处理异常
        callback(msg);
    }
}

}