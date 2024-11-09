//
// Created on 2024/11/6.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "napi/native_api.h"
#include "client_print.h"
#include "utils.h"

EXTERN_C_START
static napi_env env_prt = nullptr;
static napi_ref print_handler_ref = nullptr; // 打印回调函数: (msg) => void

napi_value 
native_set_client_print_handler(napi_env env, napi_callback_info info) {
    env_prt = env;
    
    if ( print_handler_ref != nullptr ) {
        napi_delete_reference(env_prt, print_handler_ref);
    }
    
    size_t argc = 1;
    int print_handler_index = 0;
    napi_value args[argc];
    // 获取传入的参数
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    // 获取打印处理函数
    napi_create_reference(env, args[print_handler_index], 1, &print_handler_ref);
    return nullptr;
}

/// 客户端打印消息
void client_print_message(const char *msg) {
    if ( env_prt == nullptr ) return;
    if ( print_handler_ref == nullptr ) return;
    napi_value print_handler_value;
    napi_get_reference_value(env_prt, print_handler_ref, &print_handler_value);

    // 将 C 字符串转换为 napi_value
    napi_value msg_value;
    napi_create_string_utf8(env_prt, msg, NAPI_AUTO_LENGTH, &msg_value);

    // 调用回调函数
    napi_value global_value;
    napi_get_global(env_prt, &global_value);
    napi_call_function(env_prt, global_value, print_handler_value, 1, &msg_value, nullptr);
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