//
// Created on 2024/11/9.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "log_level.h"
#include <cstring>

extern "C" {
    int native_log_get_level_by_name(const char *name);
    const char *native_log_get_level_name(int level);
    int native_log_get_level(void);
}

napi_value
native_set_log_level(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    int log_level_index = 0;
    napi_value args[argc];

    // 获取字符串长度，分配并转换为 C 字符串
    size_t name_len;
    napi_get_value_string_utf8(env, args[log_level_index], nullptr, 0, &name_len);
    char* name = new char[name_len + 1];
    napi_get_value_string_utf8(env, args[log_level_index], name, name_len + 1, &name_len);
    
    int level = native_log_get_level_by_name(name);
    napi_value level_value;
    napi_create_int32(env, level, &level_value);
    return level_value;
}

napi_value
native_get_log_level(napi_env env, napi_callback_info info) {
    int level = native_log_get_level();
    const char *name = native_log_get_level_name(level);
    napi_value name_value;
    napi_create_string_utf8(env, name, strlen(name) + 1, &name_value);
    return name_value;
}