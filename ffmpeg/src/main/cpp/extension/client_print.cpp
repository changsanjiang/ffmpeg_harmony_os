//
// Created on 2024/11/6.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "client_print.h"
#include "native_ctx.h"
#include "utils.h"

EXTERN_C_START
/// 客户端打印消息
void client_print_message(const char *msg) {
    napi_env env = native_ctx_get_env();
    if ( env == nullptr ) return;
    // 获取打印回调函数
    napi_ref print_handler_ref = native_ctx_get_print_handler_ref();
    if ( print_handler_ref == nullptr ) return;
    napi_value print_handler_value;
    napi_get_reference_value(env, print_handler_ref, &print_handler_value);

    // 将 C 字符串转换为 napi_value
    napi_value msg_value;
    napi_create_string_utf8(env, msg, NAPI_AUTO_LENGTH, &msg_value);

    // 调用回调函数
    napi_value global_value;
    napi_get_global(env, &global_value);
    napi_call_function(env, global_value, print_handler_value, 1, &msg_value, nullptr);
}

/// 客户端打印消息
void client_print_message2(const char *fmt, va_list args) {
    char *message = native_string_create(fmt, args);
    if ( message ) {
        client_print_message(message); // 打印消息
        native_string_free(&message);
    }
}

/// 客户端打印消息
void client_print_message3(const char *format, ...) {
    va_list args;
    va_start(args, format);
    client_print_message2(format, args); 
    va_end(args);
}
EXTERN_C_END