# the minimum version of CMake.
cmake_minimum_required(VERSION 3.5.0)
project(ffmpeg)

set(NATIVERENDER_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})

if(DEFINED PACKAGE_FIND_FILE)
    include(${PACKAGE_FIND_FILE})
endif()

if(OHOS_ARCH STREQUAL "arm64-v8a")
    add_definitions(-DFFAV_ARCH_ARM64_V8a)
elseif(OHOS_ARCH STREQUAL "x86_64")
    add_definitions(-DFFAV_ARCH_X86_64)
endif()

add_definitions(-DFFAV_PLATFORM_OHOS)

file(GLOB FF_SRC_FILES "napi_init.cpp" "fftools/*.c" "extension/*.c" "extension/*.cpp" "av/*.cpp" "av/**/*.cpp" "general/*.cpp")
add_library(ffmpeg SHARED ${FF_SRC_FILES})
target_include_directories(ffmpeg PRIVATE
    ${NATIVERENDER_ROOT_PATH}
)

# napi
target_link_libraries(ffmpeg PRIVATE libace_napi.z.so)
# OHAudio
target_link_libraries(ffmpeg PRIVATE libohaudio.so)
# HiLog
target_link_libraries(ffmpeg PRIVATE libhilog_ndk.z.so)
# NetConnection
target_link_libraries(ffmpeg PRIVATE libnet_connection.so)
# libz.so
target_link_libraries(ffmpeg PRIVATE libz.so)

#修改文件CMakeLists.txt
#因为此三方库中存在汇编编译的部分，所以需要修改CFLAGS参考如下，符号不可抢占且优先使用本地符号
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -Wno-int-conversion -Wl,-Bsymbolic")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -Wno-int-conversion -Wl,-Bsymbolic")
#将三方库加入工程中
target_link_libraries(ffmpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/FFmpeg/${OHOS_ARCH}/lib/libavcodec.a)
target_link_libraries(ffmpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/FFmpeg/${OHOS_ARCH}/lib/libavdevice.a)
target_link_libraries(ffmpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/FFmpeg/${OHOS_ARCH}/lib/libavfilter.a)
target_link_libraries(ffmpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/FFmpeg/${OHOS_ARCH}/lib/libavformat.a)
target_link_libraries(ffmpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/FFmpeg/${OHOS_ARCH}/lib/libavutil.a)
target_link_libraries(ffmpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/FFmpeg/${OHOS_ARCH}/lib/libpostproc.a)
target_link_libraries(ffmpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/FFmpeg/${OHOS_ARCH}/lib/libswresample.a)
target_link_libraries(ffmpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/FFmpeg/${OHOS_ARCH}/lib/libswscale.a)

target_link_libraries(ffmpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/brotli/${OHOS_ARCH}/lib/libbrotlicommon-static.a)
target_link_libraries(ffmpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/brotli/${OHOS_ARCH}/lib/libbrotlidec-static.a)
target_link_libraries(ffmpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/brotli/${OHOS_ARCH}/lib/libbrotlienc-static.a)
target_link_libraries(ffmpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/bzip2_1_0_8/${OHOS_ARCH}/lib/libbz2.a)
target_link_libraries(ffmpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/fontconfig/${OHOS_ARCH}/lib/libfontconfig.a)
target_link_libraries(ffmpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/freetype2/${OHOS_ARCH}/lib/libfreetype.a)
target_link_libraries(ffmpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/fribidi/${OHOS_ARCH}/lib/libfribidi.a)
target_link_libraries(ffmpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/harfbuzz/${OHOS_ARCH}/lib/libharfbuzz.a)
target_link_libraries(ffmpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/lame/${OHOS_ARCH}/lib/libmp3lame.a)
target_link_libraries(ffmpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libaom/${OHOS_ARCH}/lib/libaom.a)
target_link_libraries(ffmpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libass/${OHOS_ARCH}/lib/libass.a)
target_link_libraries(ffmpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libogg/${OHOS_ARCH}/lib/libogg.a)
target_link_libraries(ffmpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libpng/${OHOS_ARCH}/lib/libpng16.a)
target_link_libraries(ffmpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libunibreak/${OHOS_ARCH}/lib/libunibreak.a)
target_link_libraries(ffmpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libvorbis/${OHOS_ARCH}/lib/libvorbis.a)
target_link_libraries(ffmpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libvorbis/${OHOS_ARCH}/lib/libvorbisenc.a)
target_link_libraries(ffmpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libvorbis/${OHOS_ARCH}/lib/libvorbisfile.a)
target_link_libraries(ffmpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libxml2/${OHOS_ARCH}/lib/libxml2.a)
target_link_libraries(ffmpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/opencore-amr/${OHOS_ARCH}/lib/libopencore-amrnb.a)
target_link_libraries(ffmpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/opencore-amr/${OHOS_ARCH}/lib/libopencore-amrwb.a)
target_link_libraries(ffmpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/openssl_3_4_0/${OHOS_ARCH}/lib/libcrypto.a)
target_link_libraries(ffmpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/openssl_3_4_0/${OHOS_ARCH}/lib/libssl.a)
target_link_libraries(ffmpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/opus/${OHOS_ARCH}/lib/libopus.a)
target_link_libraries(ffmpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/rubberband/${OHOS_ARCH}/lib/librubberband.a)
target_link_libraries(ffmpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/x264/${OHOS_ARCH}/lib/libx264.a)
target_link_libraries(ffmpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/x265/${OHOS_ARCH}/lib/libx265.a)
target_link_libraries(ffmpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/xz/${OHOS_ARCH}/lib/liblzma.a)
target_link_libraries(ffmpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/zimg/${OHOS_ARCH}/lib/libzimg.a)

#将三方库的头文件加入工程中
target_include_directories(ffmpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/FFmpeg/${OHOS_ARCH}/include)