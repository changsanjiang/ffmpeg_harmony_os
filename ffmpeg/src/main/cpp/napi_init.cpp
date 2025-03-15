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
#include "general/FFAbortController.h"
#if __has_include("general/FFAudioMultiStreamPlayer.h")
#include "general/FFAudioMultiStreamPlayer.h"
#endif
#include "general/FFAudioPlayer.h"
#include "general/FFAudioWriter.h"
#include "general/FFPlayWhenReadyChangeReason.h"
#include "general/FFmpeg.h"
#include "napi/native_api.h"

EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports)
{    
    FFAV::FFmpeg::Init(env, exports);
    FFAV::FFAudioPlayer::Init(env, exports);
    FFAV::FFAbortController::Init(env, exports);
    FFAV::FFPlayWhenReadyChangeReason::Init(env, exports);
#if __has_include("general/FFAudioMultiStreamPlayer.h")
    FFAV::FFAudioMultiStreamPlayer::Init(env, exports);
#endif
    FFAV::FFAudioWriter::Init(env, exports);
    return exports;
}
EXTERN_C_END

static napi_module demoModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "ffmpeg",
    .nm_priv = ((void*)0),
    .reserved = { 0 },
};

extern "C" __attribute__((constructor)) void RegisterFfmpegModule(void)
{
    napi_module_register(&demoModule);
}