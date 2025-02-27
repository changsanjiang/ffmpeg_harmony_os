//
// Created on 2025/2/25.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "TaskScheduler.h"
#include <thread>

namespace FFAV {

std::shared_ptr<TaskScheduler> TaskScheduler::scheduleTask(Task task, int delay_in_seconds) {
    std::shared_ptr<TaskScheduler> scheduler = std::make_shared<TaskScheduler>();
    scheduler->callback = task;
    scheduler->delay_in_seconds = delay_in_seconds;
    scheduler->future = std::async(std::launch::async, std::bind(&TaskScheduler::schedule, scheduler));
    return scheduler;
}

bool TaskScheduler::tryCancelTask() {
    std::lock_guard<std::mutex> lock(mtx);
    if ( has_started ) {
        return false;
    }
    is_cancelled = true;
    return true;
}

void TaskScheduler::waitForCompletion() {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [this] { return has_finished || is_cancelled; });
}

void TaskScheduler::schedule() {
    std::this_thread::sleep_for(std::chrono::seconds(delay_in_seconds));  
    {
        std::lock_guard<std::mutex> lock(mtx);
        if ( is_cancelled ) {
            return;  
        }
        has_started = true;
    }

    callback();  // 执行任务
    
    {
        std::lock_guard<std::mutex> lock(mtx);
        has_finished = true;  
    }
    cv.notify_all();
}

}