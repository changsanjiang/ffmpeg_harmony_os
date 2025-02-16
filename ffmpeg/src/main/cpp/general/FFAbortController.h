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

#ifndef FFMPEGPROJ_FFABORTCONTROLLER_H
#define FFMPEGPROJ_FFABORTCONTROLLER_H

#include "napi/native_api.h"
#include <mutex>

namespace FFAV {

class FFAbortController {
public:
    static napi_value Init(napi_env env, napi_value exports);
    
private:
    static void Destructor(napi_env env, void* nativeObject, void* finalize_hint);
    
    static napi_value New(napi_env env, napi_callback_info info);
    static napi_value GetSignal(napi_env env, napi_callback_info info);
    static napi_value Abort(napi_env env, napi_callback_info info);
    
    napi_ref signal_ref;
};


class FFAbortSignal {
public:
    static napi_value New(napi_env env, napi_callback_info info);
    void abort(napi_env env, napi_value error);
    
    using AbortedCallback = std::function<void(napi_ref reason_ref)>;
    void setAbortedCallback(AbortedCallback callback);
    
private:
    static void Destructor(napi_env env, void* nativeObject, void* finalize_hint);
    static napi_value GetAborted(napi_env env, napi_callback_info info);
    static napi_value GetReason(napi_env env, napi_callback_info info);
    static napi_value AddEventListener(napi_env env, napi_callback_info info);
    static void InvokeCallback(napi_env env, napi_value js_callback, void* context, void* data);

    std::mutex mtx;
    bool aborted = false;
    napi_ref reason_ref = nullptr;
    std::vector<napi_threadsafe_function> callback_refs; // [(error: Error) => void]
    AbortedCallback aborted_callback = nullptr;
};

}
#endif //FFMPEGPROJ_FFABORTCONTROLLER_H
