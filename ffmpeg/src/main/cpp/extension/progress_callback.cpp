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
// Created on 2024/11/9.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "progress_callback.h"
#include "native_ctx.h"

EXTERN_C_START
void 
native_report_progress(const char *message) {
    napi_env env = native_ctx_get_env();
    if ( env == nullptr ) return;
    
    // 获取打印回调函数
    napi_ref progress_callback_ref = native_ctx_get_progress_callback_ref();
    if ( progress_callback_ref == nullptr ) return;
    napi_value progress_callback_value;
    napi_get_reference_value(env, progress_callback_ref, &progress_callback_value);
    
    // 将 C 字符串转换为 napi_value
    napi_value msg_value;
    napi_create_string_utf8(env, message, NAPI_AUTO_LENGTH, &msg_value);

    // 调用回调函数
    napi_value global_value;
    if (napi_get_global(env, &global_value) != napi_ok) return;
    napi_call_function(env, global_value, progress_callback_value, 1, &msg_value, nullptr);
}
EXTERN_C_END