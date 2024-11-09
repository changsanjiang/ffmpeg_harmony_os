//
// Created on 2024/11/7.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef UTILITIES_EXECUTECOMMANDS_H
#define UTILITIES_EXECUTECOMMANDS_H
#include "napi/native_api.h"

EXTERN_C_START
napi_value
native_exe_prepare(napi_env env, napi_callback_info info); // info: (executionId: number)

napi_value
native_exe_cmds(napi_env env, napi_callback_info info); // info: (executionId: number, cmds: string[], log_callback: (level, msg) => void, progress_callback: (msg) => void)

napi_value
native_exe_cancel(napi_env env, napi_callback_info info); // info: (executionId: number)
EXTERN_C_END

#endif //UTILITIES_EXECUTECOMMANDS_H
