//
// Created on 2025/2/25.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "TaskScheduler.h"

#include <thread>
#include <future>

namespace FFAV {
std::shared_ptr<TaskScheduler> TaskScheduler::scheduleTask(Task task, int delayInSeconds) {
    auto scheduler = std::make_shared<TaskScheduler>();
    std::async(std::launch::async, [scheduler, delayInSeconds, task] {
        std::this_thread::sleep_for(std::chrono::seconds(delayInSeconds));  
        {
            std::lock_guard<std::mutex> lock(scheduler->mtx);
            if ( scheduler->is_cancelled ) {
                return;  
            }
            scheduler->has_started = true;
        }
    
        task();  // 执行任务
        
        {
            std::lock_guard<std::mutex> lock(scheduler->mtx);
            scheduler->has_finished = true;  
        }
        scheduler->cv.notify_all();  
    });
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

}