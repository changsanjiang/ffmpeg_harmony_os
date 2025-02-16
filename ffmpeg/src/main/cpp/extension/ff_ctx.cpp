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

EXTERN_C_START

_Thread_local static struct {
    napi_threadsafe_function log_callback_ref;
    napi_threadsafe_function progress_callback_ref;
    napi_threadsafe_function output_callback_ref;
} ff_ctx = { nullptr, nullptr, nullptr }; 

void 
ff_ctx_init(
    napi_threadsafe_function log_callback_ref,
    napi_threadsafe_function progress_callback_ref,
    napi_threadsafe_function output_callback_ref
) {
    ff_ctx.log_callback_ref = log_callback_ref;
    ff_ctx.progress_callback_ref = progress_callback_ref;
    ff_ctx.output_callback_ref = output_callback_ref;
}

napi_threadsafe_function
ff_ctx_get_log_callback_ref(void) {
    return ff_ctx.log_callback_ref;
}

napi_threadsafe_function
ff_ctx_get_progress_callback_ref(void) {
    return ff_ctx.progress_callback_ref;
}

napi_threadsafe_function
ff_ctx_get_output_callback_ref(void) {
    return ff_ctx.output_callback_ref;    
}

void 
ff_ctx_release(void) {
    if ( ff_ctx.log_callback_ref ) {
        napi_release_threadsafe_function(ff_ctx.log_callback_ref, napi_tsfn_release);
        ff_ctx.log_callback_ref = nullptr;
    }
    
    if ( ff_ctx.progress_callback_ref ) {
        napi_release_threadsafe_function(ff_ctx.progress_callback_ref, napi_tsfn_release);
        ff_ctx.progress_callback_ref = nullptr;
    }
    
    if ( ff_ctx.output_callback_ref ) {
        napi_release_threadsafe_function(ff_ctx.output_callback_ref, napi_tsfn_release);
        ff_ctx.output_callback_ref = nullptr;
    }
}
EXTERN_C_END
