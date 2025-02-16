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
// Created on 2024/11/6.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "napi/native_api.h"
#include "client_print.h"
#include "utils.h"
#include <cstddef>
#include <stdio.h>
#include <string.h>
#include "hilog/log.h"

EXTERN_C_START
static bool ENABLED_PRINT = false;

napi_value
NAPI_Set_EnabledClientPrint(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    int enabled_value_index = 0;
    napi_value args[argc];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    bool enabled;
    napi_get_value_bool(env, args[enabled_value_index], &enabled);
    ENABLED_PRINT = enabled;
    return nullptr;
}        

#undef LOG_TAG
#define LOG_TAG "SJ_FFMPEG"  

/// 客户端打印消息
void client_print_message(const char *msg) {
    if ( ENABLED_PRINT ) {
        OH_LOG_INFO(LOG_APP, msg, NULL);
    }
}

/// 客户端打印消息
void client_print_message2(const char *fmt, va_list args) {
    char *message = native_string_create(fmt, args);
    if ( message ) {
        client_print_message(message); // 打印消息
        native_string_free(&message);
    }
}

/// 客户端打印消息
void client_print_message3(const char *format, ...) {
    va_list args;
    va_start(args, format);
    client_print_message2(format, args); 
    va_end(args);
}
EXTERN_C_END