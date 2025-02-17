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

#include "FFAbortController.h"

namespace FFAV {
napi_value FFAbortSignal::New(napi_env env, napi_callback_info info) {
    napi_value js_this;
    napi_create_object(env, &js_this);
    
    napi_property_descriptor properties[] = {
        { "aborted", nullptr, nullptr, GetAborted, nullptr, nullptr, napi_default, nullptr},
        { "reason", nullptr, nullptr, GetReason, nullptr, nullptr, napi_default, nullptr},
        { "addEventListener", nullptr, AddEventListener, nullptr, nullptr, nullptr, napi_default, nullptr},
    };
    
    size_t property_count = sizeof(properties) / sizeof(properties[0]);
    napi_define_properties(env, js_this, property_count, properties);
    
    FFAbortSignal* obj = new FFAbortSignal();
    napi_wrap(env, js_this, reinterpret_cast<void*>(obj), FFAbortSignal::Destructor, nullptr, nullptr);
    return js_this;
}

void FFAbortSignal::abort(napi_env env, napi_value error) {
    std::unique_lock<std::mutex> lock(mtx);
    if ( aborted ) {
        return;
    }
    
    aborted = true;
    
    napi_valuetype errortype;
    napi_typeof(env, error, &errortype);
    if ( errortype == napi_undefined ) {
        napi_value error_code, error_msg;
        napi_create_string_utf8(env, "FF_ABORT_ERR", NAPI_AUTO_LENGTH, &error_code);
        napi_create_string_utf8(env, "The operation was aborted", NAPI_AUTO_LENGTH, &error_msg);
        napi_create_type_error(env, error_code, error_msg, &error);
    }
    napi_create_reference(env, error, 1 + callback_refs.size(), &reason_ref);
    
    if ( aborted_callback ) {
        aborted_callback(reason_ref);
        aborted_callback = nullptr;
    }
    
    if ( !callback_refs.empty() ) {
        auto refs(callback_refs);
        callback_refs.clear();
        
        napi_ref error_ref = reason_ref;
        lock.unlock();
        for ( auto ref : refs ) {
            napi_call_threadsafe_function(ref, error_ref, napi_tsfn_nonblocking);
            napi_release_threadsafe_function(ref, napi_tsfn_release);
        }
    }
}

void FFAbortSignal::Destructor(napi_env env, void* nativeObject, void* finalize_hint) {
    FFAbortSignal* obj = reinterpret_cast<FFAbortSignal*>(nativeObject);
    std::lock_guard<std::mutex> lock(obj->mtx);
   
    if ( !obj->callback_refs.empty() ) {
        for ( auto ref : obj->callback_refs ) {
            napi_release_threadsafe_function(ref, napi_tsfn_release);
        }
        obj->callback_refs.clear();
    } 
    
    if ( obj->reason_ref ) {
        napi_reference_unref(env, obj->reason_ref, nullptr);
    }
    delete obj;
}

napi_value FFAbortSignal::GetAborted(napi_env env, napi_callback_info info) {
    napi_value js_this;
    napi_get_cb_info(env, info, nullptr, nullptr, &js_this, nullptr);

    FFAbortSignal* obj;
    napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
    napi_value aborted;
    std::lock_guard<std::mutex> lock(obj->mtx);
    napi_get_boolean(env, obj->aborted, &aborted);
    return aborted;
}

napi_value FFAbortSignal::GetReason(napi_env env, napi_callback_info info) {
    napi_value js_this;
    napi_get_cb_info(env, info, nullptr, nullptr, &js_this, nullptr);

    FFAbortSignal* obj;
    napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
    
    napi_value reason;
    std::lock_guard<std::mutex> lock(obj->mtx);
    if ( obj->aborted ) {
        napi_get_reference_value(env, obj->reason_ref, &reason);
    }
    else {
        napi_get_undefined(env, &reason);
    }
    return reason;
}

napi_value FFAbortSignal::AddEventListener(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    int callback_idx = 1;

    napi_value args[argc];
    napi_value js_this;
    napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);
    
    FFAbortSignal* obj;
    napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
    
    std::unique_lock<std::mutex> lock(obj->mtx);
    if ( obj->aborted ) {
        napi_value global, error;
        napi_get_global(env, &global);
        napi_get_reference_value(env, obj->reason_ref, &error);
        lock.unlock();
        napi_call_function(env, global, args[callback_idx], 1, &error, nullptr);
    }
    else {
        napi_threadsafe_function callback_ref;
        napi_value async_resource_name;
        napi_create_string_utf8(env, "Signal aborted callback", NAPI_AUTO_LENGTH, &async_resource_name);
        napi_create_threadsafe_function(env, args[callback_idx], nullptr, async_resource_name, 0, 1, nullptr, nullptr, nullptr, FFAbortSignal::InvokeCallback, &callback_ref);
    }
    return nullptr;
}

void FFAbortSignal::setAbortedCallback(FFAbortSignal::AbortedCallback callback) {
    std::unique_lock<std::mutex> lock(mtx);
    aborted_callback = callback;
    if ( aborted && callback ) callback(reason_ref);
}

void FFAbortSignal::InvokeCallback(napi_env env, napi_value js_callback, void* context, void* data) {
    napi_ref error_ref = reinterpret_cast<napi_ref>(data);
    napi_value global, error;
    napi_get_global(env, &global);
    napi_get_reference_value(env, error_ref, &error);
    napi_call_function(env, global, js_callback, 1, &error, nullptr);
    napi_reference_unref(env, error_ref, nullptr);
}


// FFAbortController

napi_value FFAbortController::Init(napi_env env, napi_value exports) {
    napi_property_descriptor properties[] = {
        { "signal", nullptr, nullptr, GetSignal, nullptr, nullptr, napi_default, nullptr},
        { "abort", nullptr, Abort, nullptr, nullptr, nullptr, napi_default, nullptr},
    };
    size_t property_count = sizeof(properties) / sizeof(properties[0]);
    napi_value cons;
    napi_define_class(env, "FFAbortController", NAPI_AUTO_LENGTH, New, nullptr, property_count, properties, &cons);
    napi_set_named_property(env, exports, "FFAbortController", cons);
    return exports;
}

void FFAbortController::Destructor(napi_env env, void *nativeObject, void *finalize_hint) {     
    FFAbortController* obj = reinterpret_cast<FFAbortController*>(nativeObject);
    napi_delete_reference(env, obj->signal_ref);
    delete obj;
}

napi_value FFAbortController::New(napi_env env, napi_callback_info info) {
    napi_value new_target;
    napi_get_new_target(env, info, &new_target);
    if ( new_target == nullptr ) {
        napi_throw_error(env, nullptr, "FFAbortController must be called with 'new'");
        return nullptr;
    }

    napi_value js_this;
    napi_get_cb_info(env, info, nullptr, nullptr, &js_this, nullptr);
    
    // 创建 C++ 对象 
    FFAbortController* obj = new FFAbortController();
    // 通过 napi_wrap 将 ArkTS 对象 this 与 C++ 对象 obj 绑定
    napi_wrap(env, js_this, reinterpret_cast<void*>(obj), FFAbortController::Destructor, nullptr, nullptr);
    
    napi_value signal = FFAbortSignal::New(env, info);
    napi_create_reference(env, signal, 1, &obj->signal_ref);
    return js_this;
}

napi_value FFAbortController::GetSignal(napi_env env, napi_callback_info info) {
    napi_value js_this;
    napi_get_cb_info(env, info, nullptr, nullptr, &js_this, nullptr);

    FFAbortController* obj;
    napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));

    napi_value signal;
    napi_get_reference_value(env, obj->signal_ref, &signal);
    return signal;
}

napi_value FFAbortController::Abort(napi_env env, napi_callback_info info) {
    napi_value signal = GetSignal(env, info);
    FFAbortSignal* obj;
    napi_unwrap(env, signal, reinterpret_cast<void**>(&obj));
    
    size_t argc = 1;
    int error_idx = 0;
    
    napi_value args[argc];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    // abort
    obj->abort(env, args[error_idx]);
    return nullptr;
}

}