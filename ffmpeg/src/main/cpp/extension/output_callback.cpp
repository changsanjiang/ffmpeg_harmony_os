//
// Created on 2024/11/12.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "output_callback.h"
#include "native_ctx.h"

EXTERN_C_START
void 
native_report_output(const char *message) {
    napi_env env = native_ctx_get_env();
    if ( env == nullptr ) return;
    
    // 获取打印回调函数
    napi_ref output_callback_ref = native_ctx_get_output_callback_ref();
    if ( output_callback_ref == nullptr ) return;
    napi_value output_callback_value;
    napi_get_reference_value(env, output_callback_ref, &output_callback_value);
    
    // 将 C 字符串转换为 napi_value
    napi_value msg_value;
    napi_create_string_utf8(env, message, NAPI_AUTO_LENGTH, &msg_value);

    // 调用回调函数
    napi_value global_value;
    if (napi_get_global(env, &global_value) != napi_ok) return;
    napi_call_function(env, global_value, output_callback_value, 1, &msg_value, nullptr);
}
EXTERN_C_END