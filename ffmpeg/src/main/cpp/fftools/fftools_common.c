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
// Created on 2024/11/12.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "fftools_common.h"
#include "fftools/opt_common.h"
#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"
#include "libavutil_thread.h"
#include "config.h"

extern void ffmpeg_cleanup(int ret);
extern void ffprobe_cleanup(int ret);

static pthread_once_t fftools_init_once = PTHREAD_ONCE_INIT;
void _fftools_init(void) {
#if CONFIG_AVDEVICE
    avdevice_register_all();
#endif
    avformat_network_init();
    
    log_set_callback();
    
    register_ffmpeg_exit(ffmpeg_cleanup);
    register_ffprobe_exit(ffprobe_cleanup);
}

void fftools_init(void) {
    strict_pthread_once(&fftools_init_once, _fftools_init);
}