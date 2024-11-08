//
// Created on 2024/11/7.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "execute_cmds.h"
#include "extension/task.h"
#include "extension/task_mgr.h"
#include "native_ctx.h"
#include <cstdint>

EXTERN_C_START
int ffmpeg_main(_Atomic bool *is_running, int argc, char **argv);

static inline napi_value return_value(napi_env env, int result) {
    napi_value ret_value;
    napi_create_int32(env, result, &ret_value);
    return ret_value;
}

napi_value
native_exe_prepare(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    int execution_id_index = 0;
    napi_value args[argc];

    // 获取传入的参数
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    // 获取任务id
    int64_t task_id;
    napi_get_value_int64(env, args[execution_id_index], &task_id);
    
    // 创建任务
    native_task_create(task_id);
    return nullptr;
}

/// js如何将string[]传递给napi?
/// 在 JavaScript 中，可以将 string[]（即字符串数组）传递给 N-API 方法。N-API 会将 JavaScript 数组视为 napi_value 类型，然后可以在 C/C++ 代码中进一步处理。
/// 1.	JavaScript 端：直接创建字符串数组并调用 N-API 函数。
// 	2.	N-API 端：在接收端中，使用 napi_is_array 检查参数是否为数组，使用 napi_get_array_length 获取长度，接着通过 napi_get_element 逐一访问数组中的元素。
napi_value
native_exe_cmds(napi_env env, napi_callback_info info) { // info: (executionId: number, cmds: string[], log_callback: (level, msg) => void, print_handler: (msg) => void)
    size_t argc = 4;
    int execution_id_index = 0;
    int cmds_index = 1;
    int log_callback_index = 2;
    int print_handler_index = 3;
    
    napi_value args[argc];

    // 获取传入的参数
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    // 获取任务id
    int64_t task_id;
    napi_get_value_int64(env, args[execution_id_index], &task_id);
    // 获取任务
    Task *task = native_task_retain(task_id);
    if ( !task ) {
        return return_value(env, 255);
    }

    // 检查参数是否为数组
    bool isArray;
    napi_is_array(env, args[cmds_index], &isArray);
    if (!isArray) {
        napi_throw_type_error(env, nullptr, "Expected a string[] as argument");
        return return_value(env, 1);
    }

    // 获取数组长度
    uint32_t cmd_count;
    napi_get_array_length(env, args[cmds_index], &cmd_count);
    if ( cmd_count == 0 ) {
        return return_value(env, 0);
    }
    
    // 创建 C 字符串数组
    char** cmds = new char*[cmd_count];
    for (uint32_t i = 0; i < cmd_count; i++) {
        napi_value element;
        napi_get_element(env, args[cmds_index], i, &element);

        // 获取字符串长度，分配并转换为 C 字符串
        size_t str_size;
        napi_get_value_string_utf8(env, element, nullptr, 0, &str_size);
        char* cStr = new char[str_size + 1];
        napi_get_value_string_utf8(env, element, cStr, str_size + 1, &str_size);

        cmds[i] = cStr;
    }
    
    // 获取日志回调函数
    napi_ref log_callback_ref;
    napi_create_reference(env, args[log_callback_index], 1, &log_callback_ref);
    
    // 获取打印处理函数
    napi_ref print_handler_ref;
    napi_create_reference(env, args[print_handler_index], 1, &print_handler_ref);
    
    // 初始化执行上下文
    native_ctx_init(env, log_callback_ref, print_handler_ref);

    // 开始执行命令
    int result = ffmpeg_main(&task->is_running, cmd_count, cmds);
    // 任务完毕 释放内存
    for (uint32_t i = 0; i < cmd_count; i++) {
        delete[] cmds[i];
    }
    delete[] cmds;
    native_ctx_destroy();
    native_task_complete(task);
    native_task_release(task_id);
    return return_value(env, result);
}

napi_value
native_exe_cancel(napi_env env, napi_callback_info info) { // info: (executionId: number)
    size_t argc = 1;
    int execution_id_index = 0;
    napi_value args[argc];

    // 获取传入的参数
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    // 获取任务id
    int64_t task_id;
    napi_get_value_int64(env, args[execution_id_index], &task_id);
    
    // 获取任务
    Task *task = native_task_retain(task_id);
    if ( task ) {
        // 取消任务
        native_task_cancel(task);
        native_task_release(task_id);
    }
    return nullptr;
}
EXTERN_C_END