//
// Created on 2025/3/3.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef FFMPEG_HARMONY_OS_TASKSCHEDULER_H
#define FFMPEG_HARMONY_OS_TASKSCHEDULER_H

#include <future>
#include <mutex>
#include <memory>
#include <functional>
#include <condition_variable>

namespace FFAV {

class TaskScheduler {
public:
    using Task = std::function<void()>;
    static std::shared_ptr<TaskScheduler> scheduleTask(Task task, int delay_in_seconds);
    
    // 尝试取消任务, 取消成功后返回 true, 如果任务已经执行则返回 false;
    bool tryCancel();
    // 等待任务完成, 如果任务已取消或执行完成则直接返回;
    void wait();
    
private:
    std::mutex mtx;
    std::condition_variable cv;
    
    Task task;
    int delay_in_seconds;
    std::future<void> future;
    bool is_cancelled { false };
    bool has_started { false };
    bool has_finished { false };
};

}

#endif //FFMPEG_HARMONY_OS_TASKSCHEDULER_H
