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

#include "log_callback.h"
#include "ff_ctx.h"

EXTERN_C_START
static void 
native_log_callback(int level, const char *message) {
    ff_invoke_log_callback(level, message);
}

static char *
native_log_msg_concat(const char *prev, const char *next) {
    size_t prev_len = prev != nullptr ? strlen(prev) : 0;
    size_t next_len = next != nullptr ? strlen(next) : 0;
    if ( prev_len == 0 && next_len == 0 ) return nullptr;
    
    // 计算总长度并分配空间
    size_t total_len = prev_len + next_len + 1; // +1 预留 '\0';
    char *result = new char[total_len];
    if (prev != nullptr) {
        strcpy(result, prev);
    } 
    else {
        result[0] = '\0';
    }
    if (next != nullptr) {
        strcat(result, next);
    }
    return result;
}

void
native_log(int level, const char *next_msg) {
    _Thread_local static int prev_level = 0;
    _Thread_local static const char *msg_line = nullptr; // 记录一行的日志;
    
    // 如果level更换了, 则输出之前的log;
    if ( msg_line != nullptr && level != prev_level ) {
        native_log_callback(prev_level, msg_line);
        delete [] msg_line;
        msg_line = nullptr;
    }
    
    size_t next_len = strlen(next_msg);
    bool ends_with_newline = (next_len > 0 && next_msg[next_len - 1] == '\n');
    char *new_msg = native_log_msg_concat(msg_line, next_msg);
    delete[] msg_line;
    msg_line = new_msg;

    // 如果一行结束, 则输出拼接后的完整消息并清空 `msg_line`;
    if (ends_with_newline && msg_line != nullptr) {
        native_log_callback(level, msg_line);
        delete[] msg_line;
        msg_line = nullptr;
    } 
    else {
        prev_level = level;  // 更新日志级别;
    }
}
EXTERN_C_END