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
native_ctx_init(napi_env env, napi_ref log_callback_ref, napi_ref print_handler_ref); // log_callback_ref: (level, msg) => {}; print_handler_ref: (msg) => {};

napi_env
native_ctx_get_env(void);

napi_ref
native_ctx_get_log_callback_ref(void);

napi_ref
native_ctx_get_print_handler_ref(void);

void 
native_ctx_destroy(void);
EXTERN_C_END

#endif //UTILITIES_NATIVECONTEXT_H
