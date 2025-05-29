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
// Created on 2025/3/13.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "FFAudioWriter.h"
#include "av/ohutils/OHUtils.hpp"
#include <stdint.h>

namespace FFAV {

napi_value FFAudioWriter::Init(napi_env env, napi_value exports) {
        napi_property_descriptor properties[] = {
        { "prepare", nullptr, Prepare, nullptr, nullptr, nullptr, napi_default, nullptr},
        { "write", nullptr, Write, nullptr, nullptr, nullptr, napi_default, nullptr},
        { "close", nullptr, Close, nullptr, nullptr, nullptr, napi_default, nullptr},
        { "prepareSync", nullptr, PrepareSync, nullptr, nullptr, nullptr, napi_default, nullptr},
        { "writeSync", nullptr, WriteSync, nullptr, nullptr, nullptr, napi_default, nullptr},
        { "closeSync", nullptr, CloseSync, nullptr, nullptr, nullptr, napi_default, nullptr},
    };

    size_t property_count = sizeof(properties) / sizeof(properties[0]);
    napi_value cons;
    napi_define_class(env, "FFAudioWriter", NAPI_AUTO_LENGTH, New, nullptr, property_count, properties, &cons);
    napi_set_named_property(env, exports, "FFAudioWriter", cons);
    return exports;
}

void FFAudioWriter::Destructor(napi_env env, void* nativeObject, void* finalize_hint) {
    delete reinterpret_cast<FFAudioWriter*>(nativeObject);
}

/**
* 创建文件写入器实例。
* 
* @param filePath 输出文件路径（需包含正确的后缀，如 `.mp4`、`.aac`）。
*/
//  constructor(filePath: string);
napi_value FFAudioWriter::New(napi_env env, napi_callback_info info) {
    napi_value new_target;
    napi_get_new_target(env, info, &new_target);
    if ( new_target == nullptr ) {
        napi_throw_error(env, nullptr, "FFAudioWriter must be called with 'new'");
        return nullptr;
    }
    
    size_t argc = 1;
    int url_idx = 0;
    
    napi_value args[argc];
    napi_value js_this;
    napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);
    
    napi_valuetype urltype;
    napi_typeof(env, args[url_idx], &urltype);

   if ( urltype != napi_string ) {
        napi_throw_error(env, nullptr, "Invalid argument: filePath must be a string");
        return nullptr;
    }
    
    size_t url_size = 0;
    napi_get_value_string_utf8(env, args[url_idx], nullptr, 0, &url_size);
    if ( url_size == 0 ) {
        napi_throw_error(env, nullptr, "Invalid argument: filePath cannot be an empty string");
        return nullptr;
    }
    
    std::string new_url(url_size, '\0'); 
    napi_get_value_string_utf8(env, args[url_idx], &new_url[0], url_size + 1, &url_size);
    
    FFAudioWriter* obj = new FFAudioWriter(new_url);
    napi_wrap(env, js_this, reinterpret_cast<void*>(obj), FFAudioWriter::Destructor, nullptr, nullptr);
    return js_this;
}

napi_status FFAudioWriter::ParseStreamInfo(napi_env env, napi_value stream_info_value, FFAudioStreamInfo *stream_info) {
    napi_value prop_value;
    
    int32_t sample_rate;
    napi_get_named_property(env, stream_info_value, "samplingRate", &prop_value);
    napi_get_value_int32(env, prop_value, &sample_rate);
    
    int32_t nb_channels;
    napi_get_named_property(env, stream_info_value, "channels", &prop_value);
    napi_get_value_int32(env, prop_value, &nb_channels);
    
    int32_t sample_fmt;
    napi_get_named_property(env, stream_info_value, "sampleFormat", &prop_value);
    napi_get_value_int32(env, prop_value, &sample_fmt);
    
    int32_t encoding_type;
    napi_get_named_property(env, stream_info_value, "encodingType", &prop_value);
    napi_get_value_int32(env, prop_value, &encoding_type);
    
    // 暂不支持, 时间紧后续增加处理
    napi_valuetype prop_valuetype;
    napi_get_named_property(env, stream_info_value, "channelLayout", &prop_value);
    napi_typeof(env, prop_value, &prop_valuetype);
    if ( prop_valuetype != napi_undefined ) {
        napi_throw_error(env, nullptr, "The 'channelLayout' property is not supported yet.");
        return napi_invalid_arg;
    }
    
    AVSampleFormat in_sample_fmt = FFAV::Conversion::toAVFormat((OH_AudioStream_SampleFormat)sample_fmt);
    if ( in_sample_fmt == AV_SAMPLE_FMT_NONE ) {
        napi_throw_error(env, nullptr, "Unsupported or invalid sample format specified.");
        return napi_invalid_arg;
    }
    
    int in_sample_rate = sample_rate;
    int in_nb_channels = nb_channels;
    
    *stream_info = {
        in_sample_fmt,
        in_sample_rate,
        in_nb_channels
    };
    return napi_ok;
}

/**
* @brief 初始化。
* @returns Promise<void>  成功时解析，失败时抛出错误。
*/
//  public prepare(audioStreamInfo: audio.AudioStreamInfo): Promise<void>;
//  public prepare(audioStreamInfo: audio.AudioStreamInfo, callback: (error?: Error) => viod);
napi_value FFAudioWriter::Prepare(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    int stream_info_idx = 0;
    int callback_idx = 1;
    
    napi_value args[argc];
    napi_value js_this;
    napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

    napi_value stream_info_value = args[stream_info_idx];
    FFAudioStreamInfo stream_info;
    napi_status status = ParseStreamInfo(env, stream_info_value, &stream_info);
    if ( status != napi_ok ) {
        return nullptr;
    }
    
    napi_value callback_value = args[callback_idx];
    napi_valuetype callback_valuetype;
    napi_typeof(env, callback_value, &callback_valuetype);
    
    FFAudioWriterPrepareData* prepare_data = new FFAudioWriterPrepareData();
    napi_unwrap(env, js_this, (void **)&prepare_data->obj);
    napi_create_reference(env, js_this, 1, &prepare_data->js_this_ref);
    prepare_data->stream_info = stream_info;
    
    napi_value ret = nullptr;
    if ( callback_valuetype == napi_undefined ) {
        // ret promise
        napi_create_promise(env, &prepare_data->deferred, &ret);
    }
    else {
        // ret nullptr
        // callback
        napi_create_reference(env, callback_value, 1, &prepare_data->callback_ref);
    }
    
    napi_value async_resource_name;
    napi_create_string_utf8(env, "FFAudioWriterPrepare", NAPI_AUTO_LENGTH, &async_resource_name);
    napi_create_async_work(env, nullptr, async_resource_name, FFAudioWriter::AsyncExecuteCallback, FFAudioWriter::AsyncCompleteCallback, prepare_data, &prepare_data->async_work);
    napi_queue_async_work(env, prepare_data->async_work);
    return ret;
}

napi_value FFAudioWriter::PrepareSync(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    int stream_info_idx = 0;
    
    napi_value args[argc];
    napi_value js_this;
    napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);

    napi_value stream_info_value = args[stream_info_idx];
    FFAudioStreamInfo stream_info;
    napi_status status = ParseStreamInfo(env, stream_info_value, &stream_info);
    if ( status != napi_ok ) {
        return nullptr;
    }
    
    FFAudioWriter* obj;
    napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
    auto result = obj->onPrepare(stream_info.in_sample_fmt, stream_info.in_sample_rate, stream_info.in_nb_channels);
    if ( !result.success ) {
        napi_value error;
        CreateError(env, result.error_msg, &error);
        napi_throw(env, error);
    }
    return nullptr;
}

/**
* @brief 将传入的 PCM 数据进行编码，并写入到目标文件中。
* @param buffer PCM 数据（该数据应符合init初始化时的格式）。
* @returns Promise<void> 成功时解析，失败时抛出错误。
* @throws 若未初始化则抛出错误。
*/
//  public write(pcmBuffer: ArrayBuffer): Promise<void>;
//  public write(pcmBuffer: ArrayBuffer, callback: (error?: Error) => viod);
napi_value FFAudioWriter::Write(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    int buf_idx = 0;
    int callback_idx = 1;
    
    napi_value args[argc];
    napi_value js_this;
    napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);
    
    // 检查参数是否为ArrayBuffer
    bool isArrayBuffer = false;
    napi_is_arraybuffer(env, args[buf_idx], &isArrayBuffer);
    if ( !isArrayBuffer ) {
        napi_throw_error(env, nullptr, "Argument must be an ArrayBuffer");
        return nullptr;
    }
    
    void* buf_data = nullptr;
    size_t buf_byte_length = 0;
    napi_get_arraybuffer_info(env, args[buf_idx], &buf_data, &buf_byte_length);
    
    uint8_t* copy_buf_data = new uint8_t[buf_byte_length];
    memcpy(copy_buf_data, buf_data, buf_byte_length);
    
    napi_value callback_value = args[callback_idx];
    napi_valuetype callback_valuetype;
    napi_typeof(env, callback_value, &callback_valuetype);
    
    FFAudioWriterWriteData* write_data = new FFAudioWriterWriteData();
    napi_unwrap(env, js_this, (void **)&write_data->obj);
    napi_create_reference(env, js_this, 1, &write_data->js_this_ref);
    write_data->data = (uint8_t *)copy_buf_data;
    write_data->byteLength = buf_byte_length;
    
    napi_value ret = nullptr;
    if ( callback_valuetype == napi_undefined ) {
        // ret promise
        napi_create_promise(env, &write_data->deferred, &ret);
    }
    else {
        // ret nullptr
        // callback
        napi_create_reference(env, callback_value, 1, &write_data->callback_ref);
    }
    
    napi_value async_resource_name;
    napi_create_string_utf8(env, "FFAudioWriterWrite", NAPI_AUTO_LENGTH, &async_resource_name);
    napi_create_async_work(env, nullptr, async_resource_name, FFAudioWriter::AsyncExecuteCallback, FFAudioWriter::AsyncCompleteCallback, write_data, &write_data->async_work);
    napi_queue_async_work(env, write_data->async_work);
    return ret;
}

napi_value FFAudioWriter::WriteSync(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    int buf_idx = 0;
    
    napi_value args[argc];
    napi_value js_this;
    napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);
    
    // 检查参数是否为ArrayBuffer
    bool isArrayBuffer = false;
    napi_is_arraybuffer(env, args[buf_idx], &isArrayBuffer);
    if ( !isArrayBuffer ) {
        napi_throw_error(env, nullptr, "Argument must be an ArrayBuffer");
        return nullptr;
    }
    
    void* buf_data = nullptr;
    size_t buf_byte_length = 0;
    napi_get_arraybuffer_info(env, args[0], &buf_data, &buf_byte_length);
    
    FFAudioWriter* obj;
    napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
    auto result = obj->onWrite((uint8_t*)buf_data, buf_byte_length);
    if ( !result.success ) {
        napi_value error;
        CreateError(env, result.error_msg, &error);
        napi_throw(env, error);
    }
    return nullptr;
}


/**
* @brief 关闭写入器，并写入文件尾部，释放资源。当没有更多数据时调用关闭。
* @returns Promise<void> 成功时解析，失败时抛出错误。
* @throws 若未初始化则抛出错误。
*/
//  public close(): Promise<void>;
//  public close(callback: (error?: Error) => viod);
napi_value FFAudioWriter::Close(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    int callback_idx = 0;
    
    napi_value args[argc];
    napi_value js_this;
    napi_get_cb_info(env, info, &argc, args, &js_this, nullptr);
    
    napi_value callback_value = args[callback_idx];
    napi_valuetype callback_valuetype;
    napi_typeof(env, callback_value, &callback_valuetype);

    FFAudioWriterCloseData* close_data = new FFAudioWriterCloseData();
    napi_unwrap(env, js_this, (void **)&close_data->obj);
    napi_create_reference(env, js_this, 1, &close_data->js_this_ref);
    
    napi_value ret = nullptr;
    if ( callback_valuetype == napi_undefined ) {
        // ret promise
        napi_create_promise(env, &close_data->deferred, &ret);
    }
    else {
        // ret nullptr
        // callback
        napi_create_reference(env, callback_value, 1, &close_data->callback_ref);
    }
    
    napi_value async_resource_name;
    napi_create_string_utf8(env, "FFAudioWriterClose", NAPI_AUTO_LENGTH, &async_resource_name);
    napi_create_async_work(env, nullptr, async_resource_name, FFAudioWriter::AsyncExecuteCallback, FFAudioWriter::AsyncCompleteCallback, close_data, &close_data->async_work);
    napi_queue_async_work(env, close_data->async_work);
    return ret;
}

//  public closeSync();
napi_value FFAudioWriter::CloseSync(napi_env env, napi_callback_info info) {
    napi_value js_this;
    napi_get_cb_info(env, info, nullptr, nullptr, &js_this, nullptr);
    
    napi_deferred deferred = nullptr;
    napi_value promise = nullptr;
    napi_create_promise(env, &deferred, &promise);
    
    FFAudioWriter* obj;
    napi_unwrap(env, js_this, reinterpret_cast<void**>(&obj));
    
    auto result = obj->onClose();
    if ( !result.success ) {
        napi_value error;
        CreateError(env, result.error_msg, &error);
        napi_throw(env, error);
    }
    return nullptr;
}

void FFAudioWriter::AsyncExecuteCallback(napi_env env, void *data) {
    auto ed = reinterpret_cast<FFAudioWriterExecutionData *>(data);
    FFAudioWriter* obj = ed->obj;
    switch (ed->action) {
        case PREPARE: {
            auto pd = reinterpret_cast<FFAudioWriterPrepareData*>(ed);
            pd->result = obj->onPrepare(pd->stream_info.in_sample_fmt, pd->stream_info.in_sample_rate, pd->stream_info.in_nb_channels);
        }
            break;
        case WRITE: {
            auto wd = reinterpret_cast<FFAudioWriterWriteData*>(ed);
            wd->result = obj->onWrite(wd->data, wd->byteLength);
        }
            break;
        case CLOSE:
            ed->result = obj->onClose();
            break;
    }
}

void FFAudioWriter::AsyncCompleteCallback(napi_env env, napi_status status, void *data) {
    auto ed = reinterpret_cast<FFAudioWriterExecutionData *>(data);
    if ( ed->result.success ) {
        if ( ed->callback_ref ) {
            napi_value global, callback;
            napi_get_global(env, &global);
            napi_get_reference_value(env, ed->callback_ref, &callback);
            napi_call_function(env, global, callback, 0, nullptr, nullptr);
            napi_delete_reference(env, ed->callback_ref);
        }
        else {
            napi_resolve_deferred(env, ed->deferred, nullptr);
        }
    }
    else {
        napi_value error;
        CreateError(env, ed->result.error_msg, &error);
        if ( ed->callback_ref ) {
            napi_value global, callback;
            napi_get_global(env, &global);
            napi_get_reference_value(env, ed->callback_ref, &callback);
            napi_call_function(env, global, callback, 1, &error, nullptr);
            napi_delete_reference(env, ed->callback_ref);
        }
        else {
            napi_reject_deferred(env, ed->deferred, error);
        }
    }
    napi_delete_async_work(env, ed->async_work);
    napi_delete_reference(env, ed->js_this_ref);
    delete ed;
}

napi_status FFAudioWriter::CreateError(napi_env env, const std::string& error_msg, napi_value* result) {
    if ( !result ) return napi_invalid_arg;
    napi_value error_code, error_msg_value;
    napi_status status;

    status = napi_create_string_utf8(env, "FF_AUDIO_WRITER_ERR", NAPI_AUTO_LENGTH, &error_code);
    if ( status != napi_ok ) return status;

    status = napi_create_string_utf8(env, error_msg.c_str(), NAPI_AUTO_LENGTH, &error_msg_value);
    if ( status != napi_ok ) return status;

    status = napi_create_type_error(env, error_code, error_msg_value, result);
    return status;
}

FFAudioWriter::FFAudioWriterExecutionResult FFAudioWriter::onPrepare(
    AVSampleFormat in_sample_fmt,
    int in_sample_rate,
    int in_nb_channels
) {
    std::lock_guard<std::mutex> lock(mtx);
    if ( flags.is_prepare_invoked ) {
        return {
            false,
            "Prepare has already been invoked. "
            "Duplicate initialization is not allowed. Ensure that `prepare` is only called once."
        };
    }
    
    flags.is_prepare_invoked = true;
    
    writer = new AudioWriter();
    int ret = writer->init(file_path, in_sample_fmt, in_sample_rate, in_nb_channels);
    if ( ret < 0 ) {
        return {
            false,
            "Failed to initialize audio writer. "
            "Check if the file path is valid and ensure the input format is supported. "
            "Error details: " + std::string(av_err2str(ret))
        };
    }
    
    ret = writer->open();
    if ( ret < 0 ) {
        return {
            false,
            "Failed to open audio writer. "
            "Possible causes: insufficient file permissions, unsupported format, or file path issues. "
            "Error details: " + std::string(av_err2str(ret))
        };
    }
    
    flags.is_init_successful = true;
    return { true };
}

FFAudioWriter::FFAudioWriterExecutionResult FFAudioWriter::onWrite(
    uint8_t *data,
    size_t byteLength
) {
    std::lock_guard<std::mutex> lock(mtx);
    if ( !flags.is_init_successful ) {
        return {
            false,
            "Cannot write data because the writer is not initialized. "
            "Ensure `prepare` was successfully called before `write`."
        };
    }
    
    if ( flags.is_closed ) {
        return {
            false,
            "Cannot write data because the writer has already been closed. "
            "Ensure `write` is not called after `close`."
        };
    }
    
    int ret = writer->write(data, byteLength);
    if ( ret < 0 ) {
        return {
            false,
            "Failed to write audio data. "
            "Possible causes: insufficient memory, invalid data format, or internal errors. "
            "Error details: " + std::string(av_err2str(ret))
        };
    }
    
    return { true };
}

FFAudioWriter::FFAudioWriterExecutionResult FFAudioWriter::onClose() {
    std::lock_guard<std::mutex> lock(mtx);
    if ( !flags.is_init_successful ) {
        return {
            false,
            "Cannot close because the writer is not initialized or has already been closed. "
            "Ensure `prepare` was called successfully before `close`."
        };
    }
    
    int ret = writer->close();
    if ( ret < 0 ) {
        return {
            false,
            "Failed to close the writer. "
            "Ensure no ongoing writes and the file is accessible. "
            "Error details: " + std::string(av_err2str(ret))
        };
    }
    flags.is_closed = true;
    return { true };
}

FFAudioWriter::FFAudioWriter(const std::string& file_path): file_path(file_path) {
        
}

FFAudioWriter::~FFAudioWriter() {
    std::lock_guard<std::mutex> lock(mtx);
    if ( writer ) {
        delete writer;
    } 
}

}