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
// Created on 2024/11/7.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef UTILITIES_TASK_MGR_H
#define UTILITIES_TASK_MGR_H

#include "task.h"
#ifdef __cplusplus
extern "C" {
#endif
void
native_task_create(int64_t task_id);

// 使用之前先retain, 用完了记得release;

Task *
native_task_retain(int64_t task_id);

void 
native_task_release(int64_t task_id);
#ifdef __cplusplus
}
#endif
#endif //UTILITIES_TASK_MGR_H
