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
// Created on 2025/2/15.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef FFMPEGPROJ_FF_CTX_H
#define FFMPEGPROJ_FF_CTX_H
#include "napi/native_api.h"
#include <string>

EXTERN_C_START
#include <stdatomic.h>

typedef struct {
    atomic_int* pendingCallbacks;
    std::string msg;
    int level;
} FFCallbackData;

void 
ff_set_callback_refs(
    napi_threadsafe_function log_callback_ref,
    napi_threadsafe_function progress_callback_ref,
    napi_threadsafe_function output_callback_ref
);

void 
ff_invoke_log_callback(int level, const char *message);

void
ff_invoke_progress_callback(const char *message);

void 
ff_invoke_output_callback(const char *message);

void 
ff_wait_callbacks();

EXTERN_C_END

#endif //FFMPEGPROJ_FF_CTX_H
