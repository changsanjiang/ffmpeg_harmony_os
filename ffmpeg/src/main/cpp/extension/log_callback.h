//
// Created on 2024/11/7.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef UTILITIES_LOG_CALLBACK_H
#define UTILITIES_LOG_CALLBACK_H

#ifdef __cplusplus
extern "C" {
#endif
    void native_log(int level, const char *msg);
#ifdef __cplusplus
}
#endif

#endif //UTILITIES_LOG_CALLBACK_H
