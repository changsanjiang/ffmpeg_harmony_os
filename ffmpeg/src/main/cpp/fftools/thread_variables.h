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
// Created on 2024/11/5.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef UTILITIES_THREAD_VARIABLES_H
#define UTILITIES_THREAD_VARIABLES_H

#include <setjmp.h>

typedef struct OptionDef OptionDef;

// exit_program 不再是退出程序了, 改成了通过longjmp跳转会main函数了, 返回值通过exit_value获取;
extern _Thread_local jmp_buf exit_jump_buffer;
extern _Thread_local int exit_value;

extern _Thread_local OptionDef *options;

extern _Thread_local int program_log_level;
extern _Thread_local int program_log_flags;

extern _Thread_local const char *program_name;
#endif //UTILITIES_THREAD_VARIABLES_H
