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
// Created on 2025/2/11.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef FFMPEGPROJ_EVENTMESSAGE_H
#define FFMPEGPROJ_EVENTMESSAGE_H

#include "PlayWhenReadyChangeReason.h"
#include "Error.h"
#include <cstdint>

namespace FFAV {
enum EventType {
    MSG_PLAY_WHEN_READY_CHANGE,
    MSG_DURATION_CHANGE,
    MSG_CURRENT_TIME_CHANGE,
    MSG_PLAYABLE_DURATION_CHANGE,
    MSG_ERROR,
};

struct EventMessage { 
public: 
    EventMessage(EventType type): type(type) { }
    virtual ~EventMessage() = default;
    
    EventType type;
};

struct PlayWhenReadyChangeEventMessage: public EventMessage {
public: 
    bool play_when_ready;
    PlayWhenReadyChangeReason reason;
    PlayWhenReadyChangeEventMessage(bool play_when_ready, PlayWhenReadyChangeReason reason): EventMessage(MSG_PLAY_WHEN_READY_CHANGE), play_when_ready(play_when_ready), reason(reason) { }
};

struct DurationChangeEventMessage: public EventMessage {
public: 
    int64_t duration;
    DurationChangeEventMessage(int64_t duration): EventMessage(MSG_DURATION_CHANGE), duration(duration) { }
};

struct CurrentTimeEventMessage: public EventMessage {
public: 
    int64_t current_time;
    CurrentTimeEventMessage(int64_t current_time): EventMessage(MSG_CURRENT_TIME_CHANGE), current_time(current_time) { }
};

struct PlayableDurationChangeEventMessage: public EventMessage {
public: 
    int64_t playable_duration;
    PlayableDurationChangeEventMessage(int64_t playable_duration): EventMessage(MSG_PLAYABLE_DURATION_CHANGE), playable_duration(playable_duration) { } 
};

struct ErrorEventMessage: public EventMessage {
public: 
    std::shared_ptr<Error> error;
    ErrorEventMessage(std::shared_ptr<Error> error): EventMessage(MSG_ERROR), error(error) { }
};

}

#endif //FFMPEGPROJ_EVENTMESSAGE_H
