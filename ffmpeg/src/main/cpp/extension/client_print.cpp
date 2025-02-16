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

#undef LOG_TAG
#define LOG_TAG "FFAV_LOG"  

/// 客户端打印消息
void client_print_message(const char *msg) {
    OH_LOG_INFO(LOG_APP, msg, NULL);
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