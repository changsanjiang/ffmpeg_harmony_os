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