//
// Created on 2024/11/7.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "native_ctx.h"

EXTERN_C_START
typedef struct {
    napi_env env;
    napi_ref log_callback_ref;
    napi_ref print_handler_ref;
} NativeContext;

_Thread_local static NativeContext native_ctx = { }; 

void 
native_ctx_init(napi_env env, napi_ref log_callback_ref, napi_ref print_handler_ref) {
    native_ctx_destroy();
    native_ctx.env = env;
    native_ctx.log_callback_ref = log_callback_ref;
    native_ctx.print_handler_ref = print_handler_ref;
}

napi_env
native_ctx_get_env(void) {
    return native_ctx.env;
}

napi_ref
native_ctx_get_log_callback_ref(void) {
    return native_ctx.log_callback_ref;
}

napi_ref
native_ctx_get_print_handler_ref(void) {
    return native_ctx.print_handler_ref;
}

void 
native_ctx_destroy(void) {
    if ( native_ctx.env ) {
        if ( native_ctx.log_callback_ref ) {
            napi_delete_reference(native_ctx.env, native_ctx.log_callback_ref);
            native_ctx.log_callback_ref = nullptr;
        }
        if ( native_ctx.print_handler_ref ) {
            napi_delete_reference(native_ctx.env, native_ctx.print_handler_ref);
            native_ctx.print_handler_ref = nullptr;
        }
        native_ctx.env = nullptr;
    }
}
EXTERN_C_END