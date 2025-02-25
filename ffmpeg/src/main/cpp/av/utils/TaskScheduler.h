//
// Created on 2025/2/25.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef FFMPEG_HARMONY_OS_TASKSCHEDULER_H
#define FFMPEG_HARMONY_OS_TASKSCHEDULER_H

#include <mutex>

namespace FFAV {

class TaskScheduler {
public:
    // 延迟执行异步任务
    using Task = std::function<void()>;
    static std::shared_ptr<TaskScheduler> scheduleTask(Task task, int delayInSeconds);
 
    // 尝试取消任务, 取消成功后返回 true, 如果任务已经开始或已经完成返回 false;
    bool tryCancelTask();
    
    // 等待任务完成，如果任务已取消或执行完成则直接返回
    void waitForCompletion();

private:
    std::mutex mtx;
    std::condition_variable cv;
    bool is_cancelled { false }; 
    bool has_started { false };
    bool has_finished { false };
};

}

#endif //FFMPEG_HARMONY_OS_TASKSCHEDULER_H
