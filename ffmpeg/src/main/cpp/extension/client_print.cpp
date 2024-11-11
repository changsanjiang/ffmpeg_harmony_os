//
// Created on 2024/11/6.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "napi/native_api.h"
#include "client_print.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

EXTERN_C_START
static napi_threadsafe_function print_handler_fn = nullptr; // 打印回调函数: (msg) => void

// 主线程的回调函数
static void
InvokeJavaScriptCallback(napi_env env, napi_value js_cb, void* context, void* data) {
    char* message = (char*)data;

    // 调用 JavaScript 回调
    napi_value undefined, result;
    napi_get_undefined(env, &undefined);
    napi_create_string_utf8(env, message, NAPI_AUTO_LENGTH, &result);
    napi_call_function(env, undefined, js_cb, 1, &result, NULL);
    delete [] message;
}

napi_value 
NAPI_SetClientPrintHandler(napi_env env, napi_callback_info info) {
    if (print_handler_fn) {
        napi_release_threadsafe_function(print_handler_fn, napi_tsfn_release);
        print_handler_fn = nullptr;
    }
    
    size_t argc = 1;
    int print_handler_index = 0;
    napi_value args[argc];
    // 获取传入的参数
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    napi_valuetype valuetype;
    napi_typeof(env, args[print_handler_index], &valuetype);
    if (valuetype != napi_function) {
        napi_throw_type_error(env, nullptr, "Expected a function as the argument");
        return nullptr;
    }
    
    // 设置 async_resource_name
    napi_value async_resource_name;
    napi_create_string_utf8(env, "print_handler", NAPI_AUTO_LENGTH, &async_resource_name);

    // 创建线程安全函数
    napi_status status = napi_create_threadsafe_function(env, args[print_handler_index], nullptr, async_resource_name, 0, 1, nullptr, nullptr, nullptr, InvokeJavaScriptCallback, &print_handler_fn);

    if (status != napi_ok) {
        char msg[128];
        sprintf(msg, "Failed to create threadsafe function(%d)", status);
        napi_throw_error(env, nullptr, msg);
    }
    return nullptr;
}

/// 客户端打印消息
void client_print_message(const char *msg) {
    if ( print_handler_fn == nullptr ) return;
    char *heap_message = new char [strlen(msg) + 1];
    strcpy(heap_message, msg);
    // 调用线程安全函数，传递数据给 JavaScript 主线程
    napi_status status = napi_call_threadsafe_function(print_handler_fn, heap_message, napi_tsfn_nonblocking);
    if (status != napi_ok) {
        delete [] heap_message;
    }
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