# the minimum version of CMake.
cmake_minimum_required(VERSION 3.5.0)
project(myNpmLib)

set(NATIVERENDER_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})

if(DEFINED PACKAGE_FIND_FILE)
    include(${PACKAGE_FIND_FILE})
endif()

include_directories(${NATIVERENDER_ROOT_PATH}
                    ${NATIVERENDER_ROOT_PATH}/include)

# 自动查找 src 目录下所有 .cpp 文件
file(GLOB SOURCES "fftools/*.c" "napi_init.cpp" "extension/*.c" "extension/*.cpp")

# 自动查找 include 目录下所有 .h 文件
file(GLOB HEADERS "fftools/*.h" "extension/*.h")

add_library(ffmpeg SHARED ${SOURCES} ${HEADERS})
target_link_libraries(ffmpeg PUBLIC libace_napi.z.so)

#修改文件CMakeLists.txt
#因为此三方库中存在汇编编译的部分，所以需要修改CFLAGS参考如下，符号不可抢占且优先使用本地符号
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-int-conversion -Wl,-Bsymbolic")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-int-conversion -Wl,-Bsymbolic")
#将三方库加入工程中
target_link_libraries(ffmpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/FFmpeg/${OHOS_ARCH}/lib/libavcodec.a)
target_link_libraries(ffmpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/FFmpeg/${OHOS_ARCH}/lib/libavdevice.a)
target_link_libraries(ffmpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/FFmpeg/${OHOS_ARCH}/lib/libavfilter.a)
target_link_libraries(ffmpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/FFmpeg/${OHOS_ARCH}/lib/libavformat.a)
target_link_libraries(ffmpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/FFmpeg/${OHOS_ARCH}/lib/libavutil.a)
target_link_libraries(ffmpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/FFmpeg/${OHOS_ARCH}/lib/libswresample.a)
target_link_libraries(ffmpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/FFmpeg/${OHOS_ARCH}/lib/libswscale.a)
target_link_libraries(ffmpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/zlib/${OHOS_ARCH}/lib/libz.a)

target_link_libraries(ffmpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/lame/${OHOS_ARCH}/lib/libmp3lame.a)
target_link_libraries(ffmpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/openssl_1_0_2u/${OHOS_ARCH}/lib/libcrypto.a)
target_link_libraries(ffmpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/openssl_1_0_2u/${OHOS_ARCH}/lib/libssl.a)
target_link_libraries(ffmpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/rtmpdump/${OHOS_ARCH}/lib/librtmp.a)
#将三方库的头文件加入工程中
target_include_directories(ffmpeg PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/FFmpeg/${OHOS_ARCH}/include)
