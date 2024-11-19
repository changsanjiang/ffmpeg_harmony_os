//
// Created on 2024/11/14.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "uri_path.h"
#include <filemanagement/file_uri/oh_file_uri.h> 
#include <cstring>

napi_value
NAPI_GetPathFromUri(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    int uri_index = 0;
    napi_value args[argc];
    // 获取传入的参数
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    napi_value uri_value = args[uri_index];
    
    size_t uri_len;
    napi_get_value_string_utf8(env, uri_value, nullptr, 0, &uri_len);
    char *uri = new char [uri_len + 1];
    napi_get_value_string_utf8(env, uri_value, uri, uri_len + 1, nullptr);
    
    char *path = nullptr;
    napi_value path_value = nullptr;
    FileManagement_ErrCode ret = OH_FileUri_GetPathFromUri(uri, uri_len, &path);
    if (ret == 0 && path != nullptr) {
        napi_create_string_utf8(env, path, NAPI_AUTO_LENGTH, &path_value);
    }
    if ( path != nullptr ) {
        free(path);
    }
    return path_value;
}