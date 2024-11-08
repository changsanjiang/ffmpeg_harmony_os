//
// Created on 2024/11/6.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".
//
// 用于调试, 打印消息到客户端控制台;
//

#ifndef UTILITIES_CLIENTPRINT_H
#define UTILITIES_CLIENTPRINT_H

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif
    void client_print_message(const char *msg);
    void client_print_message2(const char *fmt, va_list args);
    void client_print_message3(const char *format, ...);
#ifdef __cplusplus
}
#endif

#endif //UTILITIES_CLIENTPRINT_H
