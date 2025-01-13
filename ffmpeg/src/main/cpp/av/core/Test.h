//
// Created on 2025/1/8.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef FFMPEGPROJ_TEST_H
#define FFMPEGPROJ_TEST_H

#include <string>
namespace CoreMedia {
    void test(const std::string& url);
}   

#endif //FFMPEGPROJ_TEST_H
