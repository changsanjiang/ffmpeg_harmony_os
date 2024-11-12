//
// Created on 2024/11/9.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".
//
// ffmpeg

#ifndef UTILITIES_PROGRESS_CALLBACK_H
#define UTILITIES_PROGRESS_CALLBACK_H

#ifdef __cplusplus
extern "C" {
#endif
    void native_report_progress(const char *progress);
#ifdef __cplusplus
}
#endif

#endif //UTILITIES_PROGRESS_CALLBACK_H
