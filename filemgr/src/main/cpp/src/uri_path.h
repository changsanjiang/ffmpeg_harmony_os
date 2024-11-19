//
// Created on 2024/11/14.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef UTILITIES_URI_PATH_H
#define UTILITIES_URI_PATH_H
#include "napi/native_api.h"

napi_value
NAPI_GetPathFromUri(napi_env env, napi_callback_info info); // uri: const char *;

#endif //UTILITIES_URI_PATH_H
