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

#include "utils.h"
#include <cstdio>
#include <new>

extern "C" char *native_string_create(const char *fmt, va_list args) {
    if (fmt == nullptr) return nullptr; // 检查 fmt 是否为空
    
    // 使用 vsnprintf 计算格式化字符串的大小
    va_list args_copy;
    va_copy(args_copy, args);  // 拷贝 va_list，以便再次使用
    int message_length = vsnprintf(nullptr, 0, fmt, args_copy);
    va_end(args_copy);  // 释放 args_copy
    
    if (message_length < 0) return NULL; // 如果计算失败则返回

    // 分配内存来存储格式化的字符串
    char *message = new (std::nothrow) char[message_length + 1];
    if (!message) return NULL;  // 检查内存分配是否成功
    
    vsnprintf(message, message_length + 1, fmt, args); // 生成格式化的字符串
    return message;
}

extern "C" void native_string_free(char **str_ptr) {
    if ( str_ptr && *str_ptr ) {
        delete [] *str_ptr;
        *str_ptr = nullptr;
    }
}