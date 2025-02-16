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
//
// 用于调试, 打印消息到客户端控制台;
//

#ifndef UTILITIES_CLIENTPRINT_H
#define UTILITIES_CLIENTPRINT_H

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif
    void client_print_message(const char *msg);
    void client_print_message2(const char *fmt, va_list args);
    void client_print_message3(const char *format, ...);
#ifdef __cplusplus
}
#endif

#endif //UTILITIES_CLIENTPRINT_H
