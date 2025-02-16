//
// Created on 2025/2/16.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef FFMPEG_HARMONY_OS_FFPLAYWHENREADYCHANGEREASON_H
#define FFMPEG_HARMONY_OS_FFPLAYWHENREADYCHANGEREASON_H

#include "napi/native_api.h"

namespace FFAV {

class FFPlayWhenReadyChangeReason {
public:
    static napi_value Init(napi_env env, napi_value exports) {
        napi_value reason_namespace;
        napi_create_object(env, &reason_namespace);
        
        napi_value reason;
        napi_create_int32(env, 0, &reason);
        napi_set_named_property(env, reason_namespace, "USER_REQUEST", reason);

        napi_create_int32(env, 1, &reason);
        napi_set_named_property(env, reason_namespace, "AUDIO_INTERRUPT_RESUME", reason);

        napi_create_int32(env, 2, &reason);
        napi_set_named_property(env, reason_namespace, "AUDIO_INTERRUPT_PAUSE", reason);

        napi_create_int32(env, 3, &reason);
        napi_set_named_property(env, reason_namespace, "AUDIO_INTERRUPT_STOP", reason);

        napi_create_int32(env, 4, &reason);
        napi_set_named_property(env, reason_namespace, "OLD_DEVICE_UNAVAILABLE", reason);
        
        napi_create_int32(env, 5, &reason);
        napi_set_named_property(env, reason_namespace, "PLAYBACK_ENDED", reason);

        napi_set_named_property(env, exports, "FFPlayWhenReadyChangeReason", reason_namespace);
        return exports;
    }
};
}

#endif //FFMPEG_HARMONY_OS_FFPLAYWHENREADYCHANGEREASON_H
