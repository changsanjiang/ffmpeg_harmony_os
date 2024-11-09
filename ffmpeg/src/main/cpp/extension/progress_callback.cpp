//
// Created on 2024/11/9.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "progress_callback.h"
#include "native_ctx.h"
#include <cstring>

EXTERN_C_START
void 
native_report_progress(const char *message) {
    
}
EXTERN_C_END