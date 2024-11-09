#include "extension/execute_cmds.h"
#include "napi/native_api.h"

// 注册napi模块 https://gitee.com/openharmony-sig/knowledge_demo_temp/blob/master/docs/napi_study/docs/hello_napi.md#%E6%B3%A8%E5%86%8Cnapi%E6%A8%A1%E5%9D%97
// 接口定义 https://gitee.com/openharmony-sig/knowledge_demo_temp/blob/master/docs/napi_study/docs/hello_napi.md#%E6%8E%A5%E5%8F%A3%E5%AE%9A%E4%B9%89
// 接口实现 https://gitee.com/openharmony-sig/knowledge_demo_temp/blob/master/docs/napi_study/docs/hello_napi.md#%E6%8E%A5%E5%8F%A3%E5%AE%9E%E7%8E%B0
// 调用接口 https://gitee.com/openharmony-sig/knowledge_demo_temp/blob/master/docs/napi_study/docs/hello_napi.md#%E8%B0%83%E7%94%A8%E6%8E%A5%E5%8F%A3

EXTERN_C_START

extern napi_value 
native_set_client_print_handler(napi_env env, napi_callback_info info);

static napi_value Init(napi_env env, napi_value exports)
{
//     typedef struct {
//         const char* utf8name;          // 属性名称（字符串）
//         napi_value name;               // 属性名称（napi_value 类型）
//         napi_callback method;          // 属性对应的 JS 回调函数
//         napi_callback getter;          // getter 函数
//         napi_callback setter;          // setter 函数
//         napi_value value;              // 直接赋给属性的值
//         napi_property_attributes attributes; // 属性的访问控制，如 napi_default、napi_enumerable
//         void* data;                    // 用于回调函数传递的数据
//     } napi_property_descriptor;
//         •	utf8name: 用于指定属性的名称（如 "myProperty"），这是一个字符串。通常可以直接用常量字符串。
//         •	attributes: 属性的访问控制。通常设置为 napi_default，其他可能用到的选项包括：
//         •	napi_enumerable: 表示属性是可枚举的。
//         •	napi_configurable: 表示属性可以被删除或重新定义。
//         •	napi_writable: 表示属性的值是可修改的（对于常量值，可能不设置此选项）。
//         •	data: 大多数场景中可以为空，仅在回调函数中需要特定数据传递时使用。
    napi_property_descriptor desc[] = {
        { "setPrintHandler", nullptr, native_set_client_print_handler, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "prepare", nullptr, native_exe_prepare, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "execute", nullptr, native_exe_cmds, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "cancel", nullptr, native_exe_cancel, nullptr, nullptr, nullptr, napi_default, nullptr },
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
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