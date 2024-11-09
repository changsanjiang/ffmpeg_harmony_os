//
// Created on 2024/11/9.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef FFMPEG_HARMONY_OS_LOG_LEVEL_H
#define FFMPEG_HARMONY_OS_LOG_LEVEL_H
#include "napi/native_api.h"

napi_value
native_set_log_level(napi_env env, napi_callback_info info); // info: (log_level: string);

napi_value
native_get_log_level(napi_env env, napi_callback_info info); // info: (void); 

#endif //FFMPEG_HARMONY_OS_LOG_LEVEL_H
