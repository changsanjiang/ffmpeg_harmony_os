//
// Created on 2024/11/7.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef UTILITIES_TASK_MGR_H
#define UTILITIES_TASK_MGR_H

#include "task.h"
#ifdef __cplusplus
extern "C" {
#endif
void
native_task_create(int64_t task_id);

// 使用之前先retain, 用完了记得release;

Task *
native_task_retain(int64_t task_id);

void 
native_task_release(int64_t task_id);
#ifdef __cplusplus
}
#endif
#endif //UTILITIES_TASK_MGR_H
