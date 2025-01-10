//
// Created on 2025/1/7.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "MediaReader.h"

namespace CoreMedia {
    static int interrupt_cb(void* ctx) {
        std::atomic<bool>* interrupt_requested = static_cast<std::atomic<bool>*>(ctx); 
        bool shouldInterrupt = interrupt_requested->load(); // 是否请求中断
        return shouldInterrupt ? 1 : 0; // 1 中断, 0 继续;
    }

    MediaReader::MediaReader(const std::string& url) : url(url), fmt_ctx(nullptr), interrupt_requested(false), interruption_mutex() {}
    
    MediaReader::~MediaReader() { release(); }
    
    int MediaReader::prepare() {
        if (fmt_ctx != nullptr) {
            return AVERROR(EAGAIN); // 已经打开，不需要再次打开
        }
        
        fmt_ctx = avformat_alloc_context();
        if ( fmt_ctx == nullptr ) {
            return AVERROR(ENOMEM);
        }
    
        fmt_ctx->interrupt_callback = { interrupt_cb, &interrupt_requested };
    
        int ret = avformat_open_input(&fmt_ctx, url.c_str(), nullptr, nullptr);
        if (ret < 0) {
            return ret;
        }
    
        return avformat_find_stream_info(fmt_ctx, nullptr);
    }
    
    int MediaReader::getStreamCount() { 
        if ( fmt_ctx == nullptr ) {
            return 0;
        }
    
        return fmt_ctx->nb_streams; 
    }
    
    AVStream *_Nullable MediaReader::getStream(int stream_index) {
        if ( fmt_ctx == nullptr ) {
            return nullptr;
        }

        if ( stream_index < 0 || stream_index >= fmt_ctx->nb_streams ) {
            return nullptr;
        }
        
        return fmt_ctx->streams[stream_index];
    }

    AVStream* _Nullable MediaReader::getBestStream(AVMediaType type) {
        return getStream(findBestStream(type));
     }

    int MediaReader::findBestStream(AVMediaType type) {
        if ( fmt_ctx == nullptr ) {
            return AVERROR_INVALIDDATA;
        }
    
        return av_find_best_stream(fmt_ctx, type, -1, -1, nullptr, 0);
    }
    
    int MediaReader::readPacket(AVPacket* _Nonnull pkt) {
        if ( fmt_ctx == nullptr ) {
            return AVERROR_INVALIDDATA;
        }
    
        std::lock_guard<std::mutex> lock(interruption_mutex);        // 读取前锁定
        return av_read_frame(fmt_ctx, pkt);
    }
    
    int MediaReader::seek(int64_t timestamp, int stream_index, int flags) {
        if ( fmt_ctx == nullptr ) {
            return AVERROR_INVALIDDATA;
        }

        if ( stream_index < 0 || stream_index >= fmt_ctx->nb_streams ) {
            return AVERROR_STREAM_NOT_FOUND;
        }

        return av_seek_frame(fmt_ctx, stream_index, timestamp, flags);
    }

    void MediaReader::interrupt() {
        interrupt_requested.store(true);
        std::lock_guard<std::mutex> lock(interruption_mutex);        // 等待读取中断        
        interrupt_requested.store(false);
    }
    
    void MediaReader::release() {
        interrupt();

        if ( fmt_ctx != nullptr ) {
            avformat_close_input(&fmt_ctx);
        }
    }
}


//
// #include <iostream>
// #include <string>
// #include <memory>
// #include <cstring>
// #include <libavfilter/avfilter.h>
// #include <libavfilter/buffersink.h>
// #include <libavfilter/buffersrc.h>
// #include <libavutil/channel_layout.h>
// #include <libavutil/opt.h>
//
// class AVFilterGraphWrapper {
// public:
//     AVFilterGraphWrapper() {
//         filter_graph = avfilter_graph_alloc();
//         if (!filter_graph) {
//             throw std::runtime_error("Failed to allocate filter graph");
//         }
//     }
//
//     ~AVFilterGraphWrapper() {
//         if (filter_graph) {
//             avfilter_graph_free(&filter_graph);
//         }
//     }
//
//     int addBufferSource(const char* args) {
//         const AVFilter* abuffersrc = avfilter_get_by_name("abuffer");
//         int ret = avfilter_graph_create_filter(&buffersrc_ctx, abuffersrc, "in", args, NULL, filter_graph);
//         if (ret < 0) {
//             std::cerr << "Cannot create audio buffer source" << std::endl;
//         }
//         return ret;
//     }
//
//     int addBufferSink() {
//         const AVFilter* abuffersink = avfilter_get_by_name("abuffersink");
//         int ret = avfilter_graph_create_filter(&buffersink_ctx, abuffersink, "out", NULL, NULL, filter_graph);
//         if (ret < 0) {
//             std::cerr << "Cannot create audio buffer sink" << std::endl;
//         }
//         return ret;
//     }
//
//     int configureSinkSampleFormat(const enum AVSampleFormat* sample_fmts) {
//         int ret = av_opt_set_int_list(buffersink_ctx, "sample_fmts", sample_fmts, -1, AV_OPT_SEARCH_CHILDREN);
//         if (ret < 0) {
//             std::cerr << "Cannot set output sample format" << std::endl;
//         }
//         return ret;
//     }
//
//     int configureSinkChannelLayout(const char* ch_layout) {
//         int ret = av_opt_set(buffersink_ctx, "ch_layouts", ch_layout, AV_OPT_SEARCH_CHILDREN);
//         if (ret < 0) {
//             std::cerr << "Cannot set output channel layout" << std::endl;
//         }
//         return ret;
//     }
//
//     int configureSinkSampleRate(const int* sample_rates) {
//         int ret = av_opt_set_int_list(buffersink_ctx, "sample_rates", sample_rates, -1, AV_OPT_SEARCH_CHILDREN);
//         if (ret < 0) {
//             std::cerr << "Cannot set output sample rate" << std::endl;
//         }
//         return ret;
//     }
//
//     int linkFilters(const char* filter_descr) {
//         AVFilterInOut* outputs = avfilter_inout_alloc();
//         AVFilterInOut* inputs = avfilter_inout_alloc();
//         if (!outputs ||!inputs) {
//             int ret = AVERROR(ENOMEM);
//             std::cerr << "Failed to allocate AVFilterInOut" << std::endl;
//             avfilter_inout_free(&outputs);
//             avfilter_inout_free(&inputs);
//             return ret;
//         }
//
//         outputs->name = av_strdup("in");
//         outputs->filter_ctx = buffersrc_ctx;
//         outputs->pad_idx = 0;
//         outputs->next = NULL;
//
//         inputs->name = av_strdup("out");
//         inputs->filter_ctx = buffersink_ctx;
//         inputs->pad_idx = 0;
//         inputs->next = NULL;
//
//         int ret = avfilter_graph_parse_ptr(filter_graph, filter_descr, &inputs, &outputs, NULL);
//         if (ret < 0) {
//             std::cerr << "Failed to parse filter graph" << std::endl;
//         } else {
//             ret = avfilter_graph_config(filter_graph, NULL);
//             if (ret < 0) {
//                 std::cerr << "Failed to configure filter graph" << std::endl;
//             }
//         }
//
//         avfilter_inout_free(&outputs);
//         avfilter_inout_free(&inputs);
//         return ret;
//     }
//
//     AVFilterContext* getBufferSourceContext() {
//         return buffersrc_ctx;
//     }
//
//     AVFilterContext* getBufferSinkContext() {
//         return buffersink_ctx;
//     }
//
// private:
//     AVFilterGraph* filter_graph = nullptr;
//     AVFilterContext* buffersrc_ctx = nullptr;
//     AVFilterContext* buffersink_ctx = nullptr;
// };
//
// #include <unistd.h>
// #include <libavcodec/avcodec.h>
// #include <libavformat/avformat.h>
// #include <libavfilter/buffersink.h>
// #include <libavfilter/buffersrc.h>
// #include <libavutil/channel_layout.h>
// #include <libavutil/opt.h>
//
// #define MAX_INPUT_FILES 5
//
// int main(int argc, char **argv) {
//     if (argc < 2) {
//         fprintf(stderr, "Usage: %s input_file1 input_file2...\n", argv[0]);
//         return 1;
//     }
//
//     AVFormatContext *fmt_ctxs[MAX_INPUT_FILES];
//     AVCodecContext *dec_ctxs[MAX_INPUT_FILES];
//     AVFilterContext *buffersrc_ctxs[MAX_INPUT_FILES];
//     AVFilterGraph *filter_graph;
//     int audio_stream_indexes[MAX_INPUT_FILES];
//     int num_input_files = argc - 1;
//
//     // 初始化滤镜图
//     filter_graph = avfilter_graph_alloc();
//     if (!filter_graph) {
//         fprintf(stderr, "Failed to allocate filter graph\n");
//         return 1;
//     }
//
//     for (int i = 0; i < num_input_files; i++) {
//         const AVCodec *dec;
//         int ret;
//
//         // 打开输入文件
//         if ((ret = avformat_open_input(&fmt_ctxs[i], argv[i + 1], NULL, NULL)) < 0) {
//             av_log(NULL, AV_LOG_ERROR, "Cannot open input file %s\n", argv[i + 1]);
//             return ret;
//         }
//
//         if ((ret = avformat_find_stream_info(fmt_ctxs[i], NULL)) < 0) {
//             av_log(NULL, AV_LOG_ERROR, "Cannot find stream information for %s\n", argv[i + 1]);
//             return ret;
//         }
//
//         // 找到音频流
//         ret = av_find_best_stream(fmt_ctxs[i], AVMEDIA_TYPE_AUDIO, -1, -1, &dec, 0);
//         if (ret < 0) {
//             av_log(NULL, AV_LOG_ERROR, "Cannot find an audio stream in %s\n", argv[i + 1]);
//             return ret;
//         }
//         audio_stream_indexes[i] = ret;
//
//         // 创建解码上下文
//         dec_ctxs[i] = avcodec_alloc_context3(dec);
//         if (!dec_ctxs[i]) {
//             return AVERROR(ENOMEM);
//         }
//         avcodec_parameters_to_context(dec_ctxs[i], fmt_ctxs[i]->streams[audio_stream_indexes[i]]->codecpar);
//
//         // 打开音频解码器
//         if ((ret = avcodec_open2(dec_ctxs[i], dec, NULL)) < 0) {
//             av_log(NULL, AV_LOG_ERROR, "Cannot open audio decoder for %s\n", argv[i + 1]);
//             return ret;
//         }
//
//         // 创建abuffer滤镜
//         const AVFilter *abuffersrc = avfilter_get_by_name("abuffer");
//         char args[512];
//         AVRational time_base = fmt_ctxs[i]->streams[audio_stream_indexes[i]]->time_base;
//         ret = snprintf(args, sizeof(args),
//                        "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=",
//                        time_base.num, time_base.den, dec_ctxs[i]->sample_rate,
//                        av_get_sample_fmt_name(dec_ctxs[i]->sample_fmt));
//         av_channel_layout_describe(&dec_ctxs[i]->ch_layout, args + ret, sizeof(args) - ret);
//         ret = avfilter_graph_create_filter(&buffersrc_ctxs[i], abuffersrc, "in_%d", args, NULL, filter_graph);
//         if (ret < 0) {
//             av_log(NULL, AV_LOG_ERROR, "Cannot create audio buffer source for %s\n", argv[i + 1]);
//             return ret;
//         }
//     }
//
//     // 使用amerge滤镜合并多路音频输入
//     const char *filter_descr = "amerge=inputs=2,aformat=sample_fmts=s16:channel_layouts=stereo";
//     AVFilterInOut *outputs[MAX_INPUT_FILES];
//     AVFilterInOut *inputs = avfilter_inout_alloc();
//     for (int i = 0; i < num_input_files; i++) {
//         outputs[i] = avfilter_inout_alloc();
//         outputs[i]->name = av_strdup(av_asprintf("in_%d", i));
//         outputs[i]->filter_ctx = buffersrc_ctxs[i];
//         outputs[i]->pad_idx = 0;
//         outputs[i]->next = (i == num_input_files - 1)? NULL : outputs[i + 1];
//     }
//
//     int ret = avfilter_graph_parse_ptr(filter_graph, filter_descr, &inputs, outputs, NULL);
//     if (ret < 0) {
//         av_log(NULL, AV_LOG_ERROR, "Failed to parse filter graph\n");
//         return ret;
//     }
//
//     ret = avfilter_graph_config(filter_graph, NULL);
//     if (ret < 0) {
//         av_log(NULL, AV_LOG_ERROR, "Failed to configure filter graph\n");
//         return ret;
//     }
//
//     // 后续处理：读取帧、滤波、输出等，代码省略
//
//     // 释放资源
//     for (int i = 0; i < num_input_files; i++) {
//         avfilter_graph_free(&filter_graph);
//         avcodec_free_context(&dec_ctxs[i]);
//         avformat_close_input(&fmt_ctxs[i]);
//     }
//     avfilter_inout_free(inputs);
//     for (int i = 0; i < num_input_files; i++) {
//         avfilter_inout_free(outputs[i]);
//     }
//
//     return 0;
// }
//
//
//
//
// #include <libavcodec/avcodec.h>
// #include <libavformat/avformat.h>
// #include <libavfilter/avfilter.h>
// #include <libavfilter/buffersink.h>
// #include <libavfilter/buffersrc.h>
// #include <libavutil/opt.h>
// #include <stdio.h>
//
// // 打开输入文件
// static int open_input_file(const char *filename, AVFormatContext **fmt_ctx) {
//     int ret;
//     if ((ret = avformat_open_input(fmt_ctx, filename, NULL, NULL)) < 0) {
//         av_log(NULL, AV_LOG_ERROR, "无法打开输入文件 %s\n", filename);
//         return ret;
//     }
//     if ((ret = avformat_find_stream_info(*fmt_ctx, NULL)) < 0) {
//         av_log(NULL, AV_LOG_ERROR, "无法找到输入文件 %s 的流信息\n", filename);
//         return ret;
//     }
//     return 0;
// }
//
// // 初始化滤镜
// static int init_filters(AVFilterGraph **filter_graph, AVFilterContext **buffersrc_ctx1,
//                         AVFilterContext **buffersrc_ctx2, AVFilterContext **buffersink_ctx,
//                         const char *filter_descr) {
//     const AVFilter *abuffersrc = avfilter_get_by_name("buffer");
//     const AVFilter *abuffersink = avfilter_get_by_name("buffersink");
//     AVFilterInOut *outputs = avfilter_inout_alloc();
//     AVFilterInOut *inputs = avfilter_inout_alloc();
//     int ret;
//
//     *filter_graph = avfilter_graph_alloc();
//     if (!*filter_graph ||!outputs ||!inputs) {
//         ret = AVERROR(ENOMEM);
//         goto end;
//     }
//
//     // 为第一个输入创建buffer source
//     char args1[512];
//     // 假设视频流，这里填充合适参数，示例简化
//     snprintf(args1, sizeof(args1), "video_size=640x480:pix_fmt=yuv420p:time_base=1/25");
//     ret = avfilter_graph_create_filter(buffersrc_ctx1, abuffersrc, "in1", args1, NULL, *filter_graph);
//     if (ret < 0) {
//         av_log(NULL, AV_LOG_ERROR, "无法创建第一个输入缓冲滤镜\n");
//         goto end;
//     }
//
//     // 为第二个输入创建buffer source
//     char args2[512];
//     // 假设视频流，这里填充合适参数，示例简化
//     snprintf(args2, sizeof(args2), "video_size=640x480:pix_fmt=yuv420p:time_base=1/25");
//     ret = avfilter_graph_create_filter(buffersrc_ctx2, abuffersrc, "in2", args2, NULL, *filter_graph);
//     if (ret < 0) {
//         av_log(NULL, AV_LOG_ERROR, "无法创建第二个输入缓冲滤镜\n");
//         goto end;
//     }
//
//     ret = avfilter_graph_create_filter(buffersink_ctx, abuffersink, "out", NULL, NULL, *filter_graph);
//     if (ret < 0) {
//         av_log(NULL, AV_LOG_ERROR, "无法创建输出缓冲滤镜\n");
//         goto end;
//     }
//
//     // 设置输出缓冲滤镜属性，示例简化
//     ret = av_opt_set(buffersink_ctx, "pix_fmts", "yuv420p", AV_OPT_SEARCH_CHILDREN);
//     if (ret < 0) {
//         av_log(NULL, AV_LOG_ERROR, "无法设置输出像素格式\n");
//         goto end;
//     }
//
//     // 连接滤镜端点
//     outputs->name = av_strdup("in1");
//     outputs->filter_ctx = *buffersrc_ctx1;
//     outputs->pad_idx = 0;
//     outputs->next = NULL;
//
//     AVFilterInOut *tmp = avfilter_inout_alloc();
//     tmp->name = av_strdup("in2");
//     tmp->filter_ctx = *buffersrc_ctx2;
//     tmp->pad_idx = 0;
//     tmp->next = NULL;
//
//     inputs->name = av_strdup("out");
//     inputs->filter_ctx = *buffersink_ctx;
//     inputs->pad_idx = 0;
//     inputs->next = NULL;
//
//     ret = avfilter_graph_parse_ptr(*filter_graph, filter_descr, &inputs, &outputs, NULL);
//     if (ret < 0) {
//         av_log(NULL, AV_LOG_ERROR, "无法解析滤镜描述\n");
//         goto end;
//     }
//
//     ret = avfilter_graph_config(*filter_graph, NULL);
//     if (ret < 0) {
//         av_log(NULL, AV_LOG_ERROR, "无法配置滤镜图\n");
//         goto end;
//     }
//
// end:
//     avfilter_inout_free(&inputs);
//     avfilter_inout_free(&outputs);
//     if (tmp) {
//         avfilter_inout_free(&tmp);
//     }
//     return ret;
// }
//
// // 主处理函数
// static int process_videos(const char *input_file1, const char *input_file2, const char *output_file) {
//     AVFormatContext *fmt_ctx1 = NULL, *fmt_ctx2 = NULL;
//     AVCodecContext *dec_ctx1 = NULL, *dec_ctx2 = NULL;
//     AVFilterGraph *filter_graph = NULL;
//     AVFilterContext *buffersrc_ctx1 = NULL, *buffersrc_ctx2 = NULL, *buffersink_ctx = NULL;
//     const char *filter_descr = "[0:v]scale=320:-1[v1];[1:v]scale=320:-1[v2];[v1][v2]hstack[outv];[0:a][1:a]amix=inputs=2:duration=first[outa]";
//     int ret;
//
//     // 打开两个输入文件
//     ret = open_input_file(input_file1, &fmt_ctx1);
//     if (ret < 0) {
//         goto end;
//     }
//     ret = open_input_file(input_file2, &fmt_ctx2);
//     if (ret < 0) {
//         goto end;
//     }
//
//     // 初始化滤镜
//     ret = init_filters(&filter_graph, &buffersrc_ctx1, &buffersrc_ctx2, &buffersink_ctx, filter_descr);
//     if (ret < 0) {
//         goto end;
//     }
//
//     // 后续处理：读取帧、滤镜处理、编码、复用写入输出文件
//     // 这里省略大量复杂代码，实际需要处理视频音频解码、滤镜应用、编码和复用
//
// end:
//     if (filter_graph) {
//         avfilter_graph_free(&filter_graph);
//     }
//     if (fmt_ctx1) {
//         avformat_close_input(&fmt_ctx1);
//     }
//     if (fmt_ctx2) {
//         avformat_close_input(&fmt_ctx2);
//     }
//     return ret;
// }
//
//
// // 初始化滤镜
// static int init_filters2(AVFilterGraph **filter_graph, AVFilterContext **buffersrc_ctx1,
//                         AVFilterContext **buffersrc_ctx2, AVFilterContext **buffersink_ctx,
//                         const char *filter_descr) {
//     const AVFilter *abuffersrc = avfilter_get_by_name("buffer");
//     const AVFilter *abuffersink = avfilter_get_by_name("buffersink");
//     AVFilterInOut *outputs = avfilter_inout_alloc();
//     AVFilterInOut *inputs = avfilter_inout_alloc();
//     AVFilterInOut *tmp = avfilter_inout_alloc();
//     int ret;
//
//     *filter_graph = avfilter_graph_alloc();
//     if (!*filter_graph ||!outputs ||!inputs ||!tmp) {
//         ret = AVERROR(ENOMEM);
//         goto end;
//     }
//
//     // 为第一个输入创建buffer source
//     char args1[512];
//     // 假设视频流，这里填充合适参数，示例简化
//     snprintf(args1, sizeof(args1), "video_size=640x480:pix_fmt=yuv420p:time_base=1/25");
//     ret = avfilter_graph_create_filter(buffersrc_ctx1, abuffersrc, "in1", args1, NULL, *filter_graph);
//     if (ret < 0) {
//         av_log(NULL, AV_LOG_ERROR, "无法创建第一个输入缓冲滤镜\n");
//         goto end;
//     }
//
//     // 为第二个输入创建buffer source
//     char args2[512];
//     // 假设视频流，这里填充合适参数，示例简化
//     snprintf(args2, sizeof(args2), "video_size=640x480:pix_fmt=yuv420p:time_base=1/25");
//     ret = avfilter_graph_create_filter(buffersrc_ctx2, abuffersrc, "in2", args2, NULL, *filter_graph);
//     if (ret < 0) {
//         av_log(NULL, AV_LOG_ERROR, "无法创建第二个输入缓冲滤镜\n");
//         goto end;
//     }
//
//     ret = avfilter_graph_create_filter(buffersink_ctx, abuffersink, "out", NULL, NULL, *filter_graph);
//     if (ret < 0) {
//         av_log(NULL, AV_LOG_ERROR, "无法创建输出缓冲滤镜\n");
//         goto end;
//     }
//
//     // 设置输出缓冲滤镜属性，示例简化
//     ret = av_opt_set(buffersink_ctx, "pix_fmts", "yuv420p", AV_OPT_SEARCH_CHILDREN);
//     if (ret < 0) {
//         av_log(NULL, AV_LOG_ERROR, "无法设置输出像素格式\n");
//         goto end;
//     }
//
//     // 连接滤镜端点
//     outputs->name = av_strdup("in1");
//     outputs->filter_ctx = *buffersrc_ctx1;
//     outputs->pad_idx = 0;
//     outputs->next = NULL;
//
//     tmp->name = av_strdup("in2");
//     tmp->filter_ctx = *buffersrc_ctx2;
//     tmp->pad_idx = 0;
//     tmp->next = NULL;
//
//     inputs->name = av_strdup("out");
//     inputs->filter_ctx = *buffersink_ctx;
//     inputs->pad_idx = 0;
//     inputs->next = NULL;
//
//     ret = avfilter_graph_parse_ptr(*filter_graph, filter_descr, &inputs, &outputs, &tmp);
//     if (ret < 0) {
//         av_log(NULL, AV_LOG_ERROR, "无法解析滤镜描述\n");
//         goto end;
//     }
//
//     ret = avfilter_graph_config(*filter_graph, NULL);
//     if (ret < 0) {
//         av_log(NULL, AV_LOG_ERROR, "无法配置滤镜图\n");
//         goto end;
//     }
//
// end:
//     avfilter_inout_free(&inputs);
//     avfilter_inout_free(&outputs);
//     avfilter_inout_free(&tmp);
//     return ret;
// }
