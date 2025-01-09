//
// Created on 2025/1/8.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "Test.h"
#include "MediaReader.h"
#include "MediaDecoder.h"
#include "extension/client_print.h"
#include <thread>
#include <string>

extern "C" {
#include <libavutil/opt.h>
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
}

namespace CoreMedia {
    void testMediaReader(const std::string& url) {
        client_print_message3("AAAA: [Test] url=%s", url.c_str());
    
        CoreMedia::MediaReader* reader = new CoreMedia::MediaReader(url);
        int ret = reader->prepare();
        client_print_message3("AAAA: [Test] prepared with status: %d", ret);
        
        int nb_streams = reader->getStreamCount();
        client_print_message3("AAAA: [Test] nb_streams: %d", nb_streams);
        
        AVPacket *pkt = av_packet_alloc();
        while ( (ret = reader->readPacket(pkt)) >= 0 ) {
            client_print_message3("AAAA: [Test] readFrame with size: %d", pkt->size);
            av_packet_unref(pkt);
        }

        client_print_message3("AAAA: [Test] clear");
        delete reader;
        av_packet_free(&pkt);
    }

    void testMediaDecoder(const std::string& url) {
        client_print_message3("AAAA: [Test] url=%s", url.c_str());
    
        CoreMedia::MediaDecoder* decoder = new CoreMedia::MediaDecoder(url);
        int ret = decoder->prepare();
        client_print_message3("AAAA: [Test] prepared with status: %d", ret);
        
        int nb_streams = decoder->getStreamCount();
        client_print_message3("AAAA: [Test] nb_streams: %d", nb_streams);
        
        for (int i = 0; i < nb_streams ; ++i ) {
            AVStream *stream = decoder->getStream(i);
            if ( stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO ) {
                decoder->selectStream(i);
                client_print_message3("AAAA: [Test] select audio stream at index: i");
                break;            
            }
        }
    
        AVFrame *frame = av_frame_alloc();
        while ( (ret = decoder->decode(frame)) >= 0 ) {
            client_print_message3("AAAA: [Test] decode frame success: nb_samples=%d, sample_rate=%d, nb_channels=%d, format=%d", frame->nb_samples, frame->sample_rate, frame->ch_layout.nb_channels, frame->format);
            av_frame_unref(frame);
        }

        client_print_message3("AAAA: [Test] clear");
        delete decoder;
        av_frame_free(&frame);
    }

    void testMediaDecoder2(const std::string &url) {
        client_print_message3("AAAA: [Test] url=%s", url.c_str());
    
        CoreMedia::MediaDecoder* decoder = new CoreMedia::MediaDecoder(url);
        int ret = decoder->prepare();
        client_print_message3("AAAA: [Test] prepared with status: %d", ret);
        
        int nb_streams = decoder->getStreamCount();
        client_print_message3("AAAA: [Test] nb_streams: %d", nb_streams);
        
        for (int i = 0; i < nb_streams ; ++i ) {
            AVStream *stream = decoder->getStream(i);
            if ( stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO ) {
                decoder->selectStream(i);
                client_print_message3("AAAA: [Test] select audio stream at index: i");
                break;            
            }
        }
        
        std::mutex mutex;
        bool interrupted = false;
    
        std::thread decode([&] { 
            AVFrame *frame = av_frame_alloc();
            int error = 0;
            while ( true ) {
                client_print_message3("AAAA: [Test] decode frame");
                error = decoder->decode(frame);
                if ( error < 0 ) {
                    client_print_message3("AAAA: [Test] decode frame error: %d, EXIT=%d", error, error == AVERROR_EXIT);
                    break;
                }
            
                client_print_message3("AAAA: [Test] decode frame success: nb_samples=%d, sample_rate=%d, nb_channels=%d, format=%d", frame->nb_samples, frame->sample_rate, frame->ch_layout.nb_channels, frame->format);
                av_frame_unref(frame);
                
                std::unique_lock<std::mutex> lock(mutex);
                if ( interrupted ) {
                    client_print_message3("AAAA: [Test] interrupted");
                    break;
                }
            }
            av_frame_free(&frame);
        });
    
        std::thread interrupt([&] {
            std::this_thread::sleep_for(std::chrono::seconds(3)); // 延迟 3 秒         
            decoder->interrupt();
            {
                std::unique_lock<std::mutex> lock(mutex);   
                interrupted = true;
            }
        
            std::this_thread::sleep_for(std::chrono::seconds(3)); // 延迟 3 秒         
        
            decoder->seek(0);
        
            AVFrame *frame = av_frame_alloc();
            int error = 0;
            int idx = 0;
            while ( (error = decoder->decode(frame)) >= 0 ) {
                client_print_message3("AAAA: [Test] 222 decode frame success: nb_samples=%d, sample_rate=%d, nb_channels=%d, format=%d", frame->nb_samples, frame->sample_rate, frame->ch_layout.nb_channels, frame->format);
                av_frame_unref(frame);
                idx += 1;
                if ( idx > 5 ) {
                    client_print_message3("AAAA: [Test] 222 break decode");
                    break;
                }
            }
            client_print_message3("AAAA: [Test] 222 decode frame error: %d, EXIT=%d", error, error == AVERROR_EXIT);
            av_frame_free(&frame);
        });
    
        decode.join();
        interrupt.join();

        client_print_message3("AAAA: [Test] clear");
        delete decoder;
    }

    void testFilterGraph(const std::string &url) {
        CoreMedia::MediaDecoder* decoder = new CoreMedia::MediaDecoder(url);
        int ret = decoder->prepare();
        if ( ret < 0 ) {
            client_print_message3("AAAA: [Test] Decoder preparation failed with status: %d", ret);
            return;
        }

        ret = decoder->selectBestStream(AVMEDIA_TYPE_AUDIO);
        if ( ret < 0 ) {
            client_print_message3("AAAA: [Test] Stream selection failed with status: %d", ret);
            return;
        }
        
        AVFilterGraph *filter_graph = avfilter_graph_alloc();
        if ( filter_graph == nullptr ) {
            client_print_message3("AAAA: [Test] Could not allocate 'filter_graph'.");
            return;
        }
    
        const AVFilter *abuffersrc  = avfilter_get_by_name("abuffer");
        const AVFilter *abuffersink = avfilter_get_by_name("abuffersink");
        /* buffer audio source: the decoded frames from the decoder will be inserted here. */
        AVFilterContext *buffersink_ctx = nullptr;
        /* buffer audio sink: to terminate the filter chain. */
        AVFilterContext *buffersrc_ctx = nullptr;
        AVFilterInOut *outputs = avfilter_inout_alloc();
        AVFilterInOut *inputs  = avfilter_inout_alloc();
    
        const char *filter_descr = "aresample=44100,aformat=sample_fmts=s16:channel_layouts=mono";
        const enum AVSampleFormat out_sample_fmts[] = { AV_SAMPLE_FMT_S16, AV_SAMPLE_FMT_NONE };
        const int out_sample_rates[] = { 44100, -1 };
        const AVFilterLink *outlink;

        ret = avfilter_graph_create_filter(&buffersrc_ctx, abuffersrc, "in", decoder->buildSrcArgs().c_str(), NULL, filter_graph);
        if (ret < 0) {
            client_print_message3("AAAA: [Test] Cannot create buffer source");
            return;
        }
    
        ret = avfilter_graph_create_filter(&buffersink_ctx, abuffersink, "out", NULL, NULL, filter_graph);
        if (ret < 0) {
            client_print_message3("AAAA: [Test] Cannot create audio buffer sink");
            return;
        }
    
        ret = av_opt_set_int_list(buffersink_ctx, "sample_fmts", out_sample_fmts, -1, AV_OPT_SEARCH_CHILDREN);
        if (ret < 0) {
            client_print_message3("AAAA: [Test] Cannot set output sample format");
            return;
        }
    
        ret = av_opt_set(buffersink_ctx, "ch_layouts", "mono", AV_OPT_SEARCH_CHILDREN);
        if (ret < 0) {
            client_print_message3("AAAA: [Test] Cannot set output channel layout");
            return;
        }
    
        ret = av_opt_set_int_list(buffersink_ctx, "sample_rates", out_sample_rates, -1, AV_OPT_SEARCH_CHILDREN);
        if (ret < 0) {
            client_print_message3("AAAA: [Test] Cannot set output sample rate");
            return;
        }
    
        /*
         * Set the endpoints for the filter graph. The filter_graph will
         * be linked to the graph described by filters_descr.
         */
    
        /*
         * The buffer source output must be connected to the input pad of
         * the first filter described by filters_descr; since the first
         * filter input label is not specified, it is set to "in" by
         * default.
         */
        outputs->name       = av_strdup("in");
        outputs->filter_ctx = buffersrc_ctx;
        outputs->pad_idx    = 0;
        outputs->next       = NULL;
    
        /*
         * The buffer sink input must be connected to the output pad of
         * the last filter described by filters_descr; since the last
         * filter output label is not specified, it is set to "out" by
         * default.
         */
        inputs->name       = av_strdup("out");
        inputs->filter_ctx = buffersink_ctx;
        inputs->pad_idx    = 0;
        inputs->next       = NULL;    
    
        ret = avfilter_graph_parse_ptr(filter_graph, filter_descr, &inputs, &outputs, NULL);
        if ( ret < 0 ) {
            client_print_message3("AAAA: [Test] filter graph parse failed with status: %d", ret);
            return;
        }

        if ( (ret = avfilter_graph_config(filter_graph, NULL)) < 0 ) {
            client_print_message3("AAAA: [Test] filter graph config failed with status: %d", ret);
            return;
        }
    
        // 检查输入和输出
        if (inputs) {
            std::string output = "AAAA: [Test] Input labels: ";
            AVFilterInOut* cur = inputs;
            while (cur) {
                output += "["; 
                output += cur->name; 
                output += ":";
                output += cur->filter_ctx->filter->name;
                output += "] ";
                cur = cur->next;
            }
            client_print_message(output.c_str());
        }
    
        if (outputs) {
            std::string output = "AAAA: [Test] Output labels: ";
            AVFilterInOut* cur = outputs;
            while (cur) {
                output += "["; 
                output += cur->name; 
                output += ":";
                output += cur->filter_ctx->filter->name;
                output += "] ";
                cur = cur->next;
            }
            client_print_message(output.c_str());
        }
    
        /* Print summary of the sink buffer
         * Note: args buffer is reused to store channel layout string */
        outlink = buffersink_ctx->inputs[0];
        char args[512];
        av_channel_layout_describe(&outlink->ch_layout, args, sizeof(args));
        client_print_message3("Output: srate:%dHz fmt:%s chlayout:%s\n", (int)outlink->sample_rate, (char *)av_x_if_null(av_get_sample_fmt_name(static_cast<AVSampleFormat>(outlink->format)), "?"), args);
    
        avfilter_inout_free(&inputs);
        avfilter_inout_free(&outputs);
    
    
        // decode 
    
        AVFrame *frame = av_frame_alloc();
        AVFrame *filt_frame = av_frame_alloc();
        while ( (ret = decoder->decode(frame)) >= 0 ) {
            client_print_message3("AAAA: [Test] decode frame success: nb_samples=%d, sample_rate=%d, nb_channels=%d, format=%d", frame->nb_samples, frame->sample_rate, frame->ch_layout.nb_channels, frame->format);
            
            // filter
            /* push the audio data from decoded frame into the filtergraph */
            if (av_buffersrc_add_frame_flags(buffersrc_ctx, frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
                client_print_message3("Error while feeding the audio filtergraph\n");
                break;
            }
            /* pull filtered audio from the filtergraph */
            while (1) {
                ret = av_buffersink_get_frame(buffersink_ctx, filt_frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                    break;
                if (ret < 0)
                    return; // goto end;
                client_print_message3("AAAA: [Test] filter frame success: nb_samples=%d, sample_rate=%d, nb_channels=%d, format=%d", filt_frame->nb_samples, filt_frame->sample_rate, filt_frame->ch_layout.nb_channels, filt_frame->format);
                av_frame_unref(filt_frame);
            }
            av_frame_unref(frame);
        }

        client_print_message3("AAAA: [Test] clear");
        delete decoder;
        av_frame_free(&frame);
    }

    void test(const std::string& url) {
        // testMediaDecoder2(url);
        testFilterGraph(url);
    }
}