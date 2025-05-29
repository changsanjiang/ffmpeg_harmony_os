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

#ifndef PRIVATE_FFMPEG_HARMONY_OS_FFAUDIOWRITER_H
#define PRIVATE_FFMPEG_HARMONY_OS_FFAUDIOWRITER_H

#include "napi/native_api.h"
#include <string>
#include <mutex>
#include "av/ffwrap/ff_audio_writer.hpp"

namespace FFAV {
/**
 * @class FFAudioWriter
 * @brief 依据文件名自动选择合适的封装格式，将 PCM 音频数据编码后写入文件。
 */
class FFAudioWriter {
public:
    static napi_value Init(napi_env env, napi_value exports);
    static void Destructor(napi_env env, void* nativeObject, void* finalize_hint);
    
private:
    //  constructor(filePath: string);
    static napi_value New(napi_env env, napi_callback_info info);
    
    //  public prepare(audioStreamInfo: audio.AudioStreamInfo): Promise<void>;
    //  public prepare(audioStreamInfo: audio.AudioStreamInfo, callback: (error?: Error) => viod);
    static napi_value Prepare(napi_env env, napi_callback_info info);
    //  public prepareSync(audioStreamInfo: audio.AudioStreamInfo);
    static napi_value PrepareSync(napi_env env, napi_callback_info info);
    
    //  public write(pcmBuffer: ArrayBuffer): Promise<void>;
    //  public write(pcmBuffer: ArrayBuffer, callback: (error?: Error) => viod);
    static napi_value Write(napi_env env, napi_callback_info info);
    //  public writeSync(pcmBuffer: ArrayBuffer);
    static napi_value WriteSync(napi_env env, napi_callback_info info);
    
    //  public close(): Promise<void>;
    //  public close(callback: (error?: Error) => viod);
    static napi_value Close(napi_env env, napi_callback_info info);
    //  public closeSync();
    static napi_value CloseSync(napi_env env, napi_callback_info info);
    
    enum FFAudioWriterExecutionAction {
        PREPARE,
        WRITE,
        CLOSE
    };
    
    struct FFAudioWriterExecutionResult {
        bool success;
        std::string error_msg;
    };
    
    struct FFAudioWriterExecutionData {
        FFAudioWriterExecutionData(FFAudioWriterExecutionAction action): action(action) { }
        virtual ~FFAudioWriterExecutionData() = default;
        
        FFAudioWriterExecutionAction action;
        FFAudioWriter* obj { nullptr }; 
        napi_ref js_this_ref { nullptr }; 
        napi_ref callback_ref { nullptr }; 
        napi_deferred deferred { nullptr };
        napi_async_work async_work { nullptr };
        FFAudioWriterExecutionResult result;
    };
    
    struct FFAudioStreamInfo {
        AVSampleFormat in_sample_fmt;
        int in_sample_rate;
        int in_nb_channels;
    };
    
    static napi_status ParseStreamInfo(napi_env env, napi_value stream_info_value, FFAudioStreamInfo *stream_info);
    
    //  public prepare(audioStreamInfo: audio.AudioStreamInfo): Promise<void>;
    struct FFAudioWriterPrepareData: FFAudioWriterExecutionData {
        FFAudioWriterPrepareData(): FFAudioWriterExecutionData(FFAudioWriterExecutionAction::PREPARE) { }
    
        FFAudioStreamInfo stream_info;
    };
    
    //  public write(pcmBuffer: ArrayBuffer): Promise<void>;
    struct FFAudioWriterWriteData: FFAudioWriterExecutionData {
        FFAudioWriterWriteData(): FFAudioWriterExecutionData(FFAudioWriterExecutionAction::WRITE) { }
        ~FFAudioWriterWriteData() {
            if ( data ) free(data);
        }
        
        uint8_t *data { nullptr };
        size_t byteLength;
    };
    
    //  public close(): Promise<void>;
    struct FFAudioWriterCloseData: FFAudioWriterExecutionData {
        FFAudioWriterCloseData(): FFAudioWriterExecutionData(FFAudioWriterExecutionAction::CLOSE) { }
    };
    
    static void AsyncExecuteCallback(napi_env env, void *data);
    static void AsyncCompleteCallback(napi_env env, napi_status status, void *data);
    static napi_status CreateError(napi_env env, const std::string& error_msg, napi_value* result);
    
    FFAudioWriter(const std::string& file_path);
    ~FFAudioWriter();
    
    std::string file_path;
    AudioWriter* writer { nullptr };
    std::mutex mtx;
    
    struct {
        unsigned int is_prepare_invoked :1;
        unsigned int is_init_successful :1;
        unsigned int is_closed :1;
    } flags { 0 };
    
    FFAudioWriterExecutionResult onPrepare(
        AVSampleFormat in_sample_fmt,
        int in_sample_rate,
        int in_nb_channels
    );
    
    FFAudioWriterExecutionResult onWrite(
        uint8_t *data,
        size_t byteLength
    );
    
    FFAudioWriterExecutionResult onClose();
};

}

#endif //PRIVATE_FFMPEG_HARMONY_OS_FFAUDIOWRITER_H
