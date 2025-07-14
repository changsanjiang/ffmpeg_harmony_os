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
// Created on 2025/1/15.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef FFMPEGPROJ_AUDIORENDERER_H
#define FFMPEGPROJ_AUDIORENDERER_H

#include <functional>
#include <ohaudio/native_audio_device_base.h>
#include <ohaudio/native_audiostream_base.h>

namespace FFAV {

class AudioRenderer {
public:    
    AudioRenderer();
    ~AudioRenderer();

    OH_AudioStream_Result init(
        OH_AudioStream_SampleFormat sample_fmt, 
        int sample_rate,
        int nb_channels, 
        OH_AudioStream_Usage usage = AUDIOSTREAM_USAGE_MUSIC
    );

    OH_AudioStream_Result play();
    OH_AudioStream_Result pause();
    OH_AudioStream_Result stop();
    OH_AudioStream_Result flush();
    OH_AudioStream_Result getCurrentState(OH_AudioStream_State* state);
    
    // [0.0, 1.0]
    OH_AudioStream_Result setVolume(float volume);
    OH_AudioStream_Result getVolume(float* volume_prt);
    
    // [0.25, 4.0]
    OH_AudioStream_Result setSpeed(float speed);
    OH_AudioStream_Result getSpeed(float* speed_ptr);
    
    OH_AudioStream_Result setDefaultOutputDevice(OH_AudioDevice_Type device_type);

    using WriteDataCallback = std::function<OH_AudioData_Callback_Result(void* _Nonnull audio_buffer, int audio_buffer_size_in_bytes)>;
    void setWriteDataCallback(WriteDataCallback callback);
    WriteDataCallback getWriteDataCallback();

    using InterruptEventCallback = std::function<void(OH_AudioInterrupt_ForceType type, OH_AudioInterrupt_Hint hint)>;
    void setInterruptEventCallback(InterruptEventCallback callback);
    InterruptEventCallback getInterruptEventCallback();
    
    using ErrorCallback = std::function<void(OH_AudioStream_Result error)>;
    void setErrorCallback(ErrorCallback callback);
    ErrorCallback getErrorCallback();

    using OutputDeviceChangeCallback = std::function<void(OH_AudioStream_DeviceChangeReason reason)>;
    void setOutputDeviceChangeCallback(OutputDeviceChangeCallback callback);
    OutputDeviceChangeCallback getOutputDeviceChangeCallback();
    
    OH_AudioStream_SampleFormat getSampleFormat();
    int getChannels();
    int getSampleRate();
    int getFrameSize(); // in bytes

private:
    OH_AudioStreamBuilder* _Nullable builder = nullptr;
    OH_AudioRenderer* _Nullable audio_renderer = nullptr;
    WriteDataCallback write_data_cb = nullptr;
    InterruptEventCallback interrupt_event_callback = nullptr;
    ErrorCallback error_callback = nullptr;
    OutputDeviceChangeCallback output_device_change_callback = nullptr;
    OH_AudioStream_SampleFormat sample_fmt = AUDIOSTREAM_SAMPLE_S16LE;
    int nb_channels = 0;
    int sample_rate = 0;
    int frame_size = 0;
    void release();
};

}

#endif //FFMPEGPROJ_AUDIORENDERER_H
