/**
 This file is part of @sj/ffmpeg.

 @sj/ffmpeg is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 @sj/ffmpeg is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with @sj/ffmpeg. If not, see <http://www.gnu.org/licenses/>.
 * */
//
// Created on 2025/2/15.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef FFMPEGPROJ_FFMPEG_H
#define FFMPEGPROJ_FFMPEG_H
#include "napi/native_api.h"

namespace FFAV {

class FFmpeg {
public:
    static napi_value Init(napi_env env, napi_value exports);
    
private:
    //  export function setFontConfigDir(dir: string);
    static napi_value SetFontConfigDir(napi_env env, napi_callback_info info);
    //  export function execute(commands: string[], options?: Options): Promise<void>;
    static napi_value Execute(napi_env env, napi_callback_info info);
    
    static void AsyncExecuteCallback(napi_env env, void *data);
    static void AsyncCompleteCallback(napi_env env, napi_status status, void *data);
    
    static void InvokeLogCallback(napi_env env, napi_value js_callback, void* context, void* data);
    static void InvokeProgressCallback(napi_env env, napi_value js_callback, void* context, void* data);
    static void InvokeOutputCallback(napi_env env, napi_value js_callback, void* context, void* data);
    
    static void SetFontConfigDefaultDir();
    static void SetEnv(const char* name, const char* value);
};

}
#endif //FFMPEGPROJ_FFMPEG_H