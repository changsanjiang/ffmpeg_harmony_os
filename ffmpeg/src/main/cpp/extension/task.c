//
// Created on 2024/11/7.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "task.h"
#include <stdatomic.h>

void native_task_complete(Task *task) {
    atomic_store(&task->is_running, false);
}

void native_task_cancel(Task *task) {
    atomic_store(&task->is_running, false);
}