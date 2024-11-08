//
// Created on 2024/11/7.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "task_mgr.h"
#include <js_native_api.h>
#include <mutex>
#include <unordered_map>

namespace {
    static std::unordered_map<int64_t, Task *> tasks;  // 存储任务 
    static std::mutex tasks_mutex;                   // 用于保护任务映射的互斥锁
};

EXTERN_C_START
void
native_task_create(int64_t task_id) {
    std::lock_guard<std::mutex> lock(tasks_mutex);
    
    // 检查任务是否已经存在
    if (tasks.find(task_id) != tasks.end()) abort();

    // 使用 new 动态分配任务对象
    Task *new_task = new (std::nothrow) Task;
    new_task->task_id = task_id;
    new_task->is_running = true;  
    new_task->ref_count = 0;

    // 将任务添加到任务容器中
    tasks[task_id] = new_task;
}

Task *
native_task_retain(int64_t task_id) {
    std::lock_guard<std::mutex> lock(tasks_mutex);
    auto it = tasks.find(task_id);
    if ( it != tasks.end() ) {
        it->second->ref_count += 1;
        return it->second;
    }
    return nullptr;
}

void
native_task_release(int64_t task_id) {
    std::lock_guard<std::mutex> lock(tasks_mutex);
    auto it = tasks.find(task_id);
    if ( it != tasks.end() ) {
        it->second->ref_count -= 1;
        if ( it->second->ref_count <= 0 ) {
            delete it->second; // 删除任务对象
            tasks.erase(it);  // 从容器中移除任务
        }
    }
}
EXTERN_C_END