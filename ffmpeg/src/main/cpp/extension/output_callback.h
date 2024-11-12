//
// Created on 2024/11/12.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".
//
// ffprobe

#ifndef UTILITIES_OUTPUT_CALLBACK_H
#define UTILITIES_OUTPUT_CALLBACK_H

#ifdef __cplusplus
extern "C" {
#endif
    void native_report_output(const char *msg);
#ifdef __cplusplus
}
#endif

#endif //UTILITIES_OUTPUT_CALLBACK_H
