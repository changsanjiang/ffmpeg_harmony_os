//
// Created on 2025/2/19.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "EventMessageQueue.h"

namespace FFAV {

EventMessageQueue::EventMessageQueue() = default;

EventMessageQueue::~EventMessageQueue() {
    // 这里上锁，确保线程安全
    std::unique_lock<std::mutex> lock(mtx);
    if ( is_running && msg_thread && msg_thread->joinable() ) {
        is_running = false;
        // 提前解锁通知等待的工作线程该退出了
        lock.unlock();
        msg_cv.notify_one();
        
        // 等待线程退出
        if ( msg_thread->joinable() ) {
            msg_thread->join();
        }
    }
}

void EventMessageQueue::setEventCallback(EventCallback callback) {
    std::unique_lock<std::mutex> lock(mtx);  
    event_callback = callback;
}

void EventMessageQueue::push(std::shared_ptr<EventMessage> msg) {
    // 这里上锁，确保线程安全
    std::unique_lock<std::mutex> lock(mtx);  
    if ( !is_running ) {
        return;
    }
    
    msg_queue.push(msg);
    
    // 创建线程延迟到收到首个消息的时候
    if ( !msg_thread ) {
        // 启动处理线程
        msg_thread = std::make_unique<std::thread>(&EventMessageQueue::ProcessQueue, this);
    }
    else {
        // 提前解锁并通知消费线程
        lock.unlock();
        msg_cv.notify_one();
    }
}

void EventMessageQueue::clear() {
    std::unique_lock<std::mutex> lock(mtx);  
    if ( !is_running ) {
        return;
    }
    while (!msg_queue.empty()) {
        msg_queue.pop();
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