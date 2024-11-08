//
// Created on 2024/11/5.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef UTILITIES_THREAD_VARIABLES_H
#define UTILITIES_THREAD_VARIABLES_H

#include <setjmp.h>

typedef struct OptionDef OptionDef;

// exit_program 不再是退出程序了, 改成了通过longjmp跳转会main函数了, 返回值通过exit_value获取;
extern _Thread_local jmp_buf exit_jump_buffer;
extern _Thread_local int exit_value;

extern _Thread_local OptionDef *options;

#endif //UTILITIES_THREAD_VARIABLES_H
