/**
    This file is part of @sj/ffmpeg.
    
    @sj/ffmpeg is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    
    @sj/ffmpeg is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with @sj/ffmpeg. If not, see <http://www.gnu.org/licenses/>.
 * */
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
#include <cstring>

EXTERN_C_START
int ffmpeg_main(_Atomic bool *is_running, int argc, char **argv);
int ffporbe_main(_Atomic bool *is_running, int argc, char **argv);

static inline napi_value return_value(napi_env env, int result) {
    napi_value ret_value;
    napi_create_int32(env, result, &ret_value);
    return ret_value;
}

napi_value
NAPI_ExePrepare(napi_env env, napi_callback_info info) {
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
NAPI_ExeCommands(napi_env env, napi_callback_info info) {
    size_t argc = 5;
    int execution_id_index = 0;
    int cmds_index = 1;
    int log_callback_index = 2;
    int progress_callback_index = 3;
    int output_callback_index = 4;
    
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
    
    bool is_ffmpeg = !strcmp(cmds[0], "ffmpeg");
    bool is_ffprobe = !is_ffmpeg && !strcmp(cmds[0], "ffprobe");
    if ( !is_ffmpeg && !is_ffprobe ) {
        napi_throw_error(env, nullptr, "Invalid command. Supported commands are 'ffmpeg' and 'ffprobe'. Please check the command name and try again.");
    }
    
    // 日志回调函数
    napi_ref log_callback_ref;
    // ffmpeg 进度回调函数
    napi_ref progress_callback_ref = NULL;
    // ffprobe 输出回调函数
    napi_ref output_callback_ref = NULL;
    
    napi_create_reference(env, args[log_callback_index], 1, &log_callback_ref);
    is_ffmpeg ? napi_create_reference(env, args[progress_callback_index], 1, &progress_callback_ref) :
                napi_create_reference(env, args[output_callback_index], 1, &output_callback_ref);
    
    // 初始化执行上下文
    native_ctx_init(env, log_callback_ref, progress_callback_ref, output_callback_ref);

    // 开始执行命令
    int result = is_ffmpeg ? ffmpeg_main(&task->is_running, cmd_count, cmds) : 
                             ffporbe_main(&task->is_running, cmd_count, cmds);
    
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
NAPI_ExeCancel(napi_env env, napi_callback_info info) { // info: (executionId: number)
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