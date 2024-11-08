//
// Created on 2024/11/5.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "thread_variables.h"
#include <stdio.h>

_Thread_local jmp_buf exit_jump_buffer;
_Thread_local int exit_value = 0;

_Thread_local OptionDef *options = NULL;

/**
 * program name, defined by the program for show_version().
 */
_Thread_local const char *program_name = NULL;

/**
 * program birth year, defined by the program for show_banner()
 */
_Thread_local int program_birth_year = 0;
