//
// Created on 2024/11/7.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef UTILITIES_UTILS_H
#define UTILITIES_UTILS_H
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif
    char *native_string_create(const char *fmt, va_list args); // should delete str after;
    void native_string_free(char **str_ptr);
#ifdef __cplusplus
}
#endif

#endif //UTILITIES_UTILS_H
