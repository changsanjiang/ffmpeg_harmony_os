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

#ifndef UTILITIES_NATIVECONTEXT_H
#define UTILITIES_NATIVECONTEXT_H
#include "napi/native_api.h"

EXTERN_C_START
void 
native_ctx_init(napi_env env, napi_ref log_callback_ref, napi_ref progress_callback_ref, napi_ref output_callback_ref); 

napi_env
native_ctx_get_env(void);

napi_ref
native_ctx_get_log_callback_ref(void); // log_callback_ref: (level, msg) => {};

napi_ref
native_ctx_get_progress_callback_ref(void); // progress_callback_ref: (msg) => {};

napi_ref
native_ctx_get_output_callback_ref(void); // output_callback_ref: (msg) => {};

void 
native_ctx_destroy(void);
EXTERN_C_END

#endif //UTILITIES_NATIVECONTEXT_H
