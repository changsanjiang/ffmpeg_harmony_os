//
// Created on 2024/11/7.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef UTILITIES_TASK_H
#define UTILITIES_TASK_H
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    int64_t task_id;
    int ref_count;
    _Atomic bool is_running;
} Task;

void native_task_complete(Task *task);
void native_task_cancel(Task *task);
#ifdef __cplusplus
}
#endif

#endif //UTILITIES_TASK_H
