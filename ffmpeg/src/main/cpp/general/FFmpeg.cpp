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

#include "FFmpeg.h"
#include <cstdint>
#include "FFAbortController.h"
#include "extension/client_print.h"
#include "extension/ff_ctx.h"

EXTERN_C_START
#include "libavutil/error.h"
#include <stdatomic.h>

int ffmpeg_main(_Atomic bool *is_running, int argc, char **argv);
int ffporbe_main(_Atomic bool *is_running, int argc, char **argv);
EXTERN_C_END

namespace FFAV { 

struct FFmpegContext {
    char** cmds = nullptr;
    uint32_t cmds_count = 0;
    bool is_ffmpeg;
    
    _Atomic bool is_running = true;
    napi_threadsafe_function log_callback_ref = nullptr; // (level: number, msg: string) => void
    napi_threadsafe_function progress_callback_ref = nullptr; // (msg: string) => void
    napi_threadsafe_function output_callback_ref = nullptr; // (msg: string) => void
    napi_ref abort_signal_ref = nullptr;
    
    napi_async_work async_work = nullptr;
    napi_deferred deferred = nullptr;
    
    int ff_ret = 0;
};

napi_value FFmpeg::Init(napi_env env, napi_value exports) {
    napi_value ffmpeg_namespace;
    napi_create_object(env, &ffmpeg_namespace);
    
    napi_property_descriptor properties[] = {
        {"execute", nullptr, Execute, nullptr, nullptr, nullptr, napi_default, nullptr}
    };

    size_t property_count = sizeof(properties) / sizeof(properties[0]);
    napi_define_properties(env, ffmpeg_namespace, property_count, properties);
    
    napi_set_named_property(env, exports, "FFmpeg", ffmpeg_namespace);
    return exports;
}

// commands: string[], options?: Options
napi_value FFmpeg::Execute(napi_env env, napi_callback_info info) {     
    napi_deferred deferred = nullptr;
    napi_value promise = nullptr;
    napi_create_promise(env, &deferred, &promise);
    
    size_t argc = 2;
    int cmds_index = 0;
    int opts_index = 1;
    
    napi_value args[argc];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    uint32_t cmds_count;
    napi_get_array_length(env, args[cmds_index], &cmds_count);
    if ( cmds_count == 0 ) {
        napi_value undefined;
        napi_get_undefined(env, &undefined);
        napi_resolve_deferred(env, deferred, undefined);
        return promise;
    }
    
    // 获取命令数组
    char** cmds = new char*[cmds_count];
    for (uint32_t i = 0; i < cmds_count; i++) {
        napi_value element;
        napi_get_element(env, args[cmds_index], i, &element);
        
        size_t element_length;
        napi_get_value_string_utf8(env, element, nullptr, 0, &element_length);
        
        char* cmd = new char[element_length + 1];
        napi_get_value_string_utf8(env, element, cmd, element_length + 1, &element_length);
        cmds[i] = cmd;
    }
    
    bool is_ffmpeg = !strcmp(cmds[0], "ffmpeg");
    bool is_ffprobe = !is_ffmpeg && !strcmp(cmds[0], "ffprobe");
    if ( !is_ffmpeg && !is_ffprobe ) {
        napi_value error, error_code, error_msg;
        napi_create_string_utf8(env, "FF_INVALID_CMD_ERR", NAPI_AUTO_LENGTH, &error_code);
        napi_create_string_utf8(env, "Invalid command. Supported commands are 'ffmpeg' and 'ffprobe'. Please check the command name and try again", NAPI_AUTO_LENGTH, &error_msg);
        napi_create_type_error(env, error_code, error_msg, &error);
        napi_reject_deferred(env, deferred, error);
        for (size_t i = 0; i < cmds_count; ++i) {
            delete[] cmds[i];
        }
        delete[] cmds;
        return promise;
    }

    // 获取配置项
    napi_threadsafe_function log_callback_ref = nullptr;
    napi_threadsafe_function progress_callback_ref = nullptr;
    napi_threadsafe_function output_callback_ref = nullptr;
    napi_ref abort_signal_ref = nullptr;

    napi_value opts = args[opts_index];
    napi_value opt_value;
    napi_valuetype opt_valuetype;
    // logCallback
    napi_get_named_property(env, opts, "logCallback", &opt_value);
    napi_typeof(env, opt_value, &opt_valuetype);
    if ( opt_valuetype == napi_function ) {
        napi_value async_resource_name;
        napi_create_string_utf8(env, "FFmpeg log callback", NAPI_AUTO_LENGTH, &async_resource_name);
        napi_create_threadsafe_function(env, opt_value, nullptr, async_resource_name, 0, 1, nullptr, nullptr, nullptr, FFmpeg::InvokeLogCallback, &log_callback_ref);
    }

    // progressCallback
    napi_get_named_property(env, opts, "progressCallback", &opt_value);
    napi_typeof(env, opt_value, &opt_valuetype);
    if ( opt_valuetype == napi_function ) {
        napi_value async_resource_name;
        napi_create_string_utf8(env, "FFmpeg progress callback", NAPI_AUTO_LENGTH, &async_resource_name);
        napi_create_threadsafe_function(env, opt_value, nullptr, async_resource_name, 0, 1, nullptr, nullptr, nullptr, FFmpeg::InvokeProgressCallback, &progress_callback_ref);
    }

    // outputCallback
    napi_get_named_property(env, opts, "outputCallback", &opt_value);
    napi_typeof(env, opt_value, &opt_valuetype);
    if ( opt_valuetype == napi_function ) {
        napi_value async_resource_name;
        napi_create_string_utf8(env, "FFmpeg output callback", NAPI_AUTO_LENGTH, &async_resource_name);
        napi_create_threadsafe_function(env, opt_value, nullptr, async_resource_name, 0, 1, nullptr, nullptr, nullptr, FFmpeg::InvokeOutputCallback, &output_callback_ref);
    }
    
    // signal
    napi_get_named_property(env, opts, "signal", &opt_value);
    napi_typeof(env, opt_value, &opt_valuetype);
    if ( opt_valuetype == napi_object ) {
        napi_create_reference(env, opt_value, 1, &abort_signal_ref);
    }
    
    FFmpegContext* ctx = new FFmpegContext();
    ctx->cmds = cmds;
    ctx->cmds_count = cmds_count;
    ctx->is_ffmpeg = is_ffmpeg;
    ctx->is_running = true;
    ctx->log_callback_ref = log_callback_ref;
    ctx->progress_callback_ref = progress_callback_ref;
    ctx->output_callback_ref = output_callback_ref;
    ctx->abort_signal_ref = abort_signal_ref;
    ctx->deferred = deferred;
    ctx->ff_ret = 0;
    
    napi_value async_resource_name;
    napi_create_string_utf8(env, "ffmpeg", NAPI_AUTO_LENGTH, &async_resource_name);
    
    // 创建异步任务
    napi_create_async_work(env, nullptr, async_resource_name, FFmpeg::AsyncExecuteCallback, FFmpeg::AsyncCompleteCallback, ctx, &ctx->async_work);
    napi_queue_async_work(env, ctx->async_work);
    return promise;
}

void FFmpeg::AsyncExecuteCallback(napi_env env, void *data) {
    FFmpegContext* ctx = reinterpret_cast<FFmpegContext *>(data);
    if ( atomic_load(&ctx->is_running) ) {
        FFAbortSignal* signal = nullptr;
        if ( ctx->abort_signal_ref ) {
            napi_value abort_signal;
            napi_get_reference_value(env, ctx->abort_signal_ref, &abort_signal);
            napi_unwrap(env, abort_signal, reinterpret_cast<void**>(&signal));
            signal->setAbortedCallback([&](napi_ref reason_ref) {
                atomic_store(&ctx->is_running, false);
            });
        }
        
        // init
        ff_ctx_init(ctx->log_callback_ref, ctx->progress_callback_ref, ctx->output_callback_ref);
        
        // execute cmds
        ctx->ff_ret = ctx->is_ffmpeg ? ffmpeg_main(&ctx->is_running, ctx->cmds_count, ctx->cmds) : 
                                       ffporbe_main(&ctx->is_running, ctx->cmds_count, ctx->cmds);
        
        if ( signal ) signal->setAbortedCallback(nullptr);
    }

    // clear partial res
    napi_delete_reference(env, ctx->abort_signal_ref);
    ff_ctx_release();
    for (size_t i = 0; i < ctx->cmds_count; ++i) {
        delete[] ctx->cmds[i];
    }
    delete[] ctx->cmds;
}

void FFmpeg::AsyncCompleteCallback(napi_env env, napi_status status, void *data) {
    FFmpegContext* ctx = reinterpret_cast<FFmpegContext *>(data);
    if ( !atomic_load(&ctx->is_running) || ctx->ff_ret == 255 ) {
        napi_value error, error_code, error_msg;
        napi_create_string_utf8(env, "FF_CANCELLED_ERR", NAPI_AUTO_LENGTH, &error_code);
        napi_create_string_utf8(env, "Execution cancelled by user", NAPI_AUTO_LENGTH, &error_msg);
        napi_create_type_error(env, error_code, error_msg, &error);
        napi_reject_deferred(env, ctx->deferred, error);
    }
    else if ( ctx->ff_ret < 0 ) {
        napi_value error, error_code, error_msg;
        napi_create_string_utf8(env, "FF_GENERIC_ERR", NAPI_AUTO_LENGTH, &error_code);
        napi_create_string_utf8(env, av_err2str(ctx->ff_ret), NAPI_AUTO_LENGTH, &error_msg);
        napi_create_type_error(env, error_code, error_msg, &error);
        napi_reject_deferred(env, ctx->deferred, error);
    }
    else {
        napi_value undefined;
        napi_get_undefined(env, &undefined);
        napi_resolve_deferred(env, ctx->deferred, undefined);
    }
    napi_delete_async_work(env, ctx->async_work);
    delete ctx;
}

// (level: number, msg: string) => void
void FFmpeg::InvokeLogCallback(napi_env env, napi_value js_callback, void* context, void* data) {
    FFLog* log = reinterpret_cast<FFLog*>(data);
    napi_value global, level, msg;
    napi_get_global(env, &global);
    napi_create_int32(env, log->level, &level);
    napi_create_string_utf8(env, log->msg.c_str(), log->msg.length(), &msg);
    napi_value args[] = { level, msg };
    napi_call_function(env, global, js_callback, 2, args, nullptr);
    delete log;
}

// (msg: string) => void
void FFmpeg::InvokeProgressCallback(napi_env env, napi_value js_callback, void* context, void* data) {
    std::string* obj = reinterpret_cast<std::string*>(data);
    napi_value global, msg;
    napi_get_global(env, &global);
    napi_create_string_utf8(env, obj->c_str(), obj->length(), &msg);
    napi_call_function(env, global, js_callback, 1, &msg, nullptr);
    delete obj;
}

// (msg: string) => void
void FFmpeg::InvokeOutputCallback(napi_env env, napi_value js_callback, void* context, void* data) {
    std::string* obj = reinterpret_cast<std::string*>(data);
    napi_value global, msg;
    napi_get_global(env, &global);
    napi_create_string_utf8(env, obj->c_str(), obj->length(), &msg);
    napi_call_function(env, global, js_callback, 1, &msg, nullptr);
    delete obj;
}

}