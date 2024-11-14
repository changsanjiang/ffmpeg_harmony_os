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

#include "thread_variables.h"
#include "libavutil/log.h"
#include <stdio.h>

_Thread_local jmp_buf exit_jump_buffer;
_Thread_local int exit_value = 0;

_Thread_local OptionDef *options = NULL;

/**
 * program name, defined by the program for show_version().
 */
_Thread_local const char *program_name = NULL;

/**
 * program birth year, defined by the program for show_banner()
 */
_Thread_local int program_birth_year = 0;

/**
 * +/-repeat: 跳过/不跳过重复日志
 * +/-level: 显示/不显示日志级别前缀
 * 
 * - 此命令会输出 debug 级别以上的日志，显示日志级别前缀，并跳过重复日志。
 *      ffmpeg -loglevel +repeat+leveldebug
 * 
 * - 此命令会输出 info 级别以上的日志，不会跳过重复日志，但会显示日志级别前缀。
 *      ffmpeg -loglevel -repeat+levelinfo
 * */
_Thread_local int program_log_level = AV_LOG_INFO;
_Thread_local int program_log_flags = AV_LOG_SKIP_REPEATED;