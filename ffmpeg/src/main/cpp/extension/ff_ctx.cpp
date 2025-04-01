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

#include "ff_ctx.h"
#include <unistd.h>  // for sleep (UNIX systems)

EXTERN_C_START

_Thread_local static struct {
    atomic_int pendingCallbacks;
    napi_threadsafe_function log_callback_ref;
    napi_threadsafe_function progress_callback_ref;
    napi_threadsafe_function output_callback_ref;
} ff_ctx = { 0, nullptr, nullptr, nullptr }; 

void 
ff_set_callback_refs(
    napi_threadsafe_function log_callback_ref,
    napi_threadsafe_function progress_callback_ref,
    napi_threadsafe_function output_callback_ref
) {
    ff_ctx.log_callback_ref = log_callback_ref;
    ff_ctx.progress_callback_ref = progress_callback_ref;
    ff_ctx.output_callback_ref = output_callback_ref;
}

void 
ff_invoke_log_callback(int level, const char *message) {
    atomic_fetch_add(&ff_ctx.pendingCallbacks, 1);
    napi_call_threadsafe_function(ff_ctx.log_callback_ref, new FFCallbackData { &ff_ctx.pendingCallbacks, message, level }, napi_tsfn_blocking);
}

void 
ff_invoke_progress_callback(const char *message) {
    atomic_fetch_add(&ff_ctx.pendingCallbacks, 1);
    napi_call_threadsafe_function(ff_ctx.progress_callback_ref, new FFCallbackData { &ff_ctx.pendingCallbacks, message, 0 }, napi_tsfn_blocking);
}

void 
ff_invoke_output_callback(const char *message) {
    atomic_fetch_add(&ff_ctx.pendingCallbacks, 1);
    napi_call_threadsafe_function(ff_ctx.output_callback_ref, new FFCallbackData { &ff_ctx.pendingCallbacks, message, 0 }, napi_tsfn_blocking);
}

void 
ff_wait_callbacks() {
    while (atomic_load(&ff_ctx.pendingCallbacks) > 0) {
        usleep(10 * 1000);  // 让出 CPU (10ms)
    }
}

EXTERN_C_END
