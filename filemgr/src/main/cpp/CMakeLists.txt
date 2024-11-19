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
file(GLOB SOURCES "napi_init.cpp" "src/*.c" "src/*.cpp")

# 自动查找 include 目录下所有 .h 文件
file(GLOB HEADERS "src/*.h")

add_library(filemgr SHARED ${SOURCES} ${HEADERS})
target_link_libraries(filemgr PUBLIC libace_napi.z.so)
target_link_libraries(filemgr PUBLIC libohfileuri.so)