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

#ifndef FFMPEGPROJ_ERROR_H
#define FFMPEGPROJ_ERROR_H

#include <ohaudio/native_audiostream_base.h>
#include <string>
extern "C" {
#include "libavutil/error.h"
}

namespace FFAV {

enum ErrorCode {
    ERR_UNKNOWN = 1000,

    ERR_INVALID_PARAM,
    ERR_ILLEGAL_STATE,
    ERR_SYSTEM,

    ERR_DECODER_NOT_FOUND,
    ERR_FILTER_NOT_FOUND,
    ERR_INVALID_DATA,
    ERR_HTTP_ERROR,
    ERR_NO_SUCH_FILE,
};

struct Error {
public:
    ErrorCode code;
    const std::string msg;
    
    Error(ErrorCode code, const std::string& msg): code(code), msg(msg) { }
    
    Error* copy() {
        return new Error(code, msg);
    }
    
    static std::shared_ptr<Error> RenderError(OH_AudioStream_Result error) {
        ErrorCode code = ERR_UNKNOWN;
        std::string msg = "Unknown error occurred";
        switch (error) {
            case AUDIOSTREAM_SUCCESS:
                break;
            case AUDIOSTREAM_ERROR_INVALID_PARAM: {
                code = ERR_INVALID_PARAM;
                msg = "Invalid parameter";
            }
                break;
            case AUDIOSTREAM_ERROR_ILLEGAL_STATE: {
                code = ERR_ILLEGAL_STATE;
                msg = "Execution status exception";
            }
                break;
            case AUDIOSTREAM_ERROR_SYSTEM: {
                code = ERR_SYSTEM;
                msg = "An system error has occurred";
            }
                break;
        }
        return std::make_shared<Error>(code, msg);    
    }
    
    static std::shared_ptr<Error> FFError(int ff_error) {
        ErrorCode code = ERR_UNKNOWN;
        std::string msg = av_err2str(ff_error);
        switch(ff_error) {
        case AVERROR_DECODER_NOT_FOUND:
            code = ERR_DECODER_NOT_FOUND;
            break;
        case AVERROR_FILTER_NOT_FOUND:
            code = ERR_FILTER_NOT_FOUND;
            break;
        case AVERROR_INVALIDDATA:
            code = ERR_INVALID_DATA;
            break;
        case AVERROR_HTTP_BAD_REQUEST:
            code = ERR_HTTP_ERROR;
            break;
        case AVERROR_HTTP_UNAUTHORIZED:
        case AVERROR_HTTP_FORBIDDEN:
        case AVERROR_HTTP_NOT_FOUND:
        case AVERROR_HTTP_OTHER_4XX:
        case AVERROR_HTTP_SERVER_ERROR:
            code = ERR_HTTP_ERROR;
            break;
        case AVERROR(ENOENT):
            code = ERR_NO_SUCH_FILE;
            break;
        }
        return std::make_shared<Error>(code, msg);    
    }
};

}

#endif //FFMPEGPROJ_ERROR_H
