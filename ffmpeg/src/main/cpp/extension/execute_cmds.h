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

#ifndef UTILITIES_EXECUTECOMMANDS_H
#define UTILITIES_EXECUTECOMMANDS_H
#include "napi/native_api.h"

EXTERN_C_START
napi_value
NAPI_ExePrepare(napi_env env, napi_callback_info info); // info: (executionId: number)

napi_value
NAPI_ExeCommands(napi_env env, napi_callback_info info); // info: (executionId: number, cmds: string[], log_callback: (level, msg) => void, progress_callback: (msg) => void)

napi_value
NAPI_ExeCancel(napi_env env, napi_callback_info info); // info: (executionId: number)
EXTERN_C_END

#endif //UTILITIES_EXECUTECOMMANDS_H
