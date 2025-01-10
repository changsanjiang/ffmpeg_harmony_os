//
// Created on 2025/1/8.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "Test.h"
#include "MediaReader.h"
#include "MediaDecoder.h"
#include "extension/client_print.h"
#include <bits/errno.h>
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
        client_print_message3("AAAA: [Test][MediaReader] prepared with status: %d", ret);
        
        int nb_streams = reader->getStreamCount();
        client_print_message3("AAAA: [Test][MediaReader] nb_streams: %d", nb_streams);
        
        AVPacket *pkt = av_packet_alloc();
        while ( (ret = reader->readPacket(pkt)) >= 0 ) {
            client_print_message3("AAAA: [Test][MediaReader] readFrame with size: %d", pkt->size);
            av_packet_unref(pkt);
        }

        client_print_message3("AAAA: [Test] clear");
        delete reader;
        av_packet_free(&pkt);
    }

    void testMediaDecoder(const std::string& url) {
        client_print_message3("AAAA: [Test] url=%s", url.c_str());

        CoreMedia::MediaReader* reader = nullptr;
        CoreMedia::MediaDecoder* decoder = nullptr;
        AVPacket *pkt = nullptr;
        AVFrame *frame = nullptr;
        int ret = 0;
        int stream_idx = -1;
    
        reader = new CoreMedia::MediaReader(url);
        ret = reader->prepare();
        client_print_message3("AAAA: [Test][MediaReader] prepared with status: %d", ret);
        if ( ret < 0 ) {
            goto end;
        }
    
        stream_idx = reader->findBestStream(AVMEDIA_TYPE_AUDIO);
        if ( stream_idx == AVERROR_STREAM_NOT_FOUND ) {
            client_print_message3("AAAA: [Test][MediaReader] Could not find audio stream");
            goto end;
        }
        
        decoder = new CoreMedia::MediaDecoder();
        ret = decoder->prepare(reader->getStream(stream_idx));
        client_print_message3("AAAA: [Test][MediaDecoder] prepared with status: %d", ret);
        if ( ret < 0 ) {
            goto end;
        }
        
        pkt = av_packet_alloc();
        frame = av_frame_alloc();
        while (ret >= 0) {
            ret = decoder->receive(frame);  // receive frame
            if ( ret >= 0 ) {
                client_print_message3("AAAA: [Test] decode frame success: nb_samples=%d, sample_rate=%d, nb_channels=%d, format=%d", frame->nb_samples, frame->sample_rate, frame->ch_layout.nb_channels, frame->format);
                av_frame_unref(frame);
            }
            else if ( ret == AVERROR(EAGAIN) ) {
                ret = reader->readPacket(pkt); // read pkt
                if ( ret < 0 ) {
                    break;
                }
            
                if ( pkt->stream_index == stream_idx ) {
                    ret = decoder->send(pkt); // send pkt
                    if ( ret < 0 ) {
                        break;
                    }
                }
                av_packet_unref(pkt);
            }
        }
        
    end: 
        client_print_message3("AAAA: [Test] end");
        if ( pkt != nullptr ) av_packet_free(&pkt);
        if ( frame != nullptr ) av_frame_free(&frame);
        if ( decoder != nullptr ) delete decoder;
        if ( reader != nullptr ) delete reader;
    }

    void testMediaDecoder2(const std::string& url) {
        client_print_message3("AAAA: [Test] url=%s", url.c_str());

        CoreMedia::MediaReader* reader = nullptr;
        CoreMedia::MediaDecoder* decoder = nullptr;
        AVPacket *pkt = nullptr;
        AVFrame *frame = nullptr;
        int ret = 0;
        int stream_idx = -1;
    
        reader = new CoreMedia::MediaReader(url);
        ret = reader->prepare();
        client_print_message3("AAAA: [Test][MediaReader] prepared with status: %s", av_err2str(ret));
        if ( ret < 0 ) {
            goto end;
        } 
    
        stream_idx = reader->findBestStream(AVMEDIA_TYPE_AUDIO);
        if ( stream_idx == AVERROR_STREAM_NOT_FOUND ) {
            client_print_message3("AAAA: [Test][MediaReader] Could not find audio stream");
            goto end;
        }
    
        decoder = new CoreMedia::MediaDecoder();
        ret = decoder->prepare(reader->getStream(stream_idx));
        client_print_message3("AAAA: [Test][MediaDecoder] prepared with status: %s", av_err2str(ret));
        if ( ret < 0 ) {
            goto end;
        }
    
        {
            std::mutex mutex;
            bool interrupted = false;

            pkt = av_packet_alloc();
            frame = av_frame_alloc();
        
            std::thread decode1([&] {
                while (ret >= 0) {
                    ret = decoder->receive(frame);  // receive frame
                    client_print_message3("AAAA: [Test][MediaDecoder] receive status: %s", av_err2str(ret));
                
                    if ( ret >= 0 ) {
                        client_print_message3("AAAA: [Test][MediaDecoder] decode frame success: nb_samples=%d, sample_rate=%d, nb_channels=%d, format=%d", frame->nb_samples, frame->sample_rate, frame->ch_layout.nb_channels, frame->format);
                        av_frame_unref(frame);
                    }
                    else if ( ret == AVERROR(EAGAIN) ) {
                        ret = reader->readPacket(pkt); // read pkt
                        client_print_message3("AAAA: [Test][MediaReader] readPacket status: %s", av_err2str(ret));
                        
                        {
                            std::unique_lock<std::mutex> lock(mutex);
                            if ( interrupted ) {
                                client_print_message3("AAAA: [Test][MediaDecoder] interrupted, EXIT=%d", ret == AVERROR_EXIT);
                                break;
                            }
                        }
                    
                        if ( ret < 0 ) {
                            break;
                        }
                    
                        if ( pkt->stream_index == stream_idx ) {
                            ret = decoder->send(pkt); // send pkt
                            client_print_message3("AAAA: [Test][MediaDecoder] send status: %s", av_err2str(ret));
                        
                            if ( ret < 0 ) {
                                break;
                            }
                        }
                        av_packet_unref(pkt);
                    }
                }
            });
        
            std::thread interrupt([&] {
                std::this_thread::sleep_for(std::chrono::seconds(3)); // 延迟 3 秒         
                client_print_message3("AAAA: [Test][MediaReader] 222 interrupt before");
                reader->interrupt();
            
                {
                    std::unique_lock<std::mutex> lock(mutex);   
                    interrupted = true;
                    client_print_message3("AAAA: [Test][MediaReader] 222 interrupt after");
                }
            
                std::this_thread::sleep_for(std::chrono::seconds(3)); // 继续延迟 3 秒         
                reader->seek(0, stream_idx);
                decoder->flush();
            
                int error = 0;
                int idx = 0;
                ret = 0;
                while (ret >= 0) {
                    ret = decoder->receive(frame);  // receive frame
                    
                    if ( ret >= 0 ) {
                        idx += 1;
                        client_print_message3("AAAA: [Test][MediaDecoder] 222 decode frame success: nb_samples=%d, sample_rate=%d, nb_channels=%d, format=%d", frame->nb_samples, frame->sample_rate, frame->ch_layout.nb_channels, frame->format);
                        if ( idx > 5 ) {
                            client_print_message3("AAAA: [Test][MediaDecoder] 222 break decode");
                            break;
                        }
                        av_frame_unref(frame);
                    }
                    else if ( ret == AVERROR(EAGAIN) ) {
                        ret = reader->readPacket(pkt); // read pkt
                        if ( ret < 0 ) {
                            break;
                        }
                    
                        if ( pkt->stream_index == stream_idx ) {
                            ret = decoder->send(pkt); // send pkt
                            if ( ret < 0 ) {
                                break;
                            }
                        }
                    
                        av_packet_unref(pkt);
                    }
                }
            
                client_print_message3("AAAA: [Test][MediaDecoder] 222 decode frame error: %d, EXIT=%d", error, error == AVERROR_EXIT);
                av_frame_free(&frame);
            });
        
            decode1.join();
            interrupt.join();
        }
        
    end: 
        client_print_message3("AAAA: [Test] end");
        if ( pkt != nullptr ) av_packet_free(&pkt);
        if ( frame != nullptr ) av_frame_free(&frame);
        if ( decoder != nullptr ) delete decoder;
        if ( reader != nullptr ) delete reader;
    }

    void testFilterGraph(const std::string &url) {
        client_print_message3("AAAA: [Test] url=%s", url.c_str());
    
        CoreMedia::MediaReader* reader = nullptr;
        CoreMedia::MediaDecoder* decoder = nullptr;
        AVPacket *pkt = nullptr;
        AVFrame *frame = nullptr;
        int stream_idx = -1;
    
        AVFilterGraph *filter_graph = nullptr;
        const AVFilter *abuffersrc  = avfilter_get_by_name("abuffer");
        const AVFilter *abuffersink = avfilter_get_by_name("abuffersink");
        /* buffer audio source: the decoded frames from the decoder will be inserted here. */
        AVFilterContext *buffersink_ctx = nullptr;
        /* buffer audio sink: to terminate the filter chain. */
        AVFilterContext *buffersrc_ctx = nullptr;
        AVFilterInOut *outputs = nullptr;
        AVFilterInOut *inputs  = nullptr;
        AVFrame *filt_frame = nullptr;
    
        const char *filter_descr = "aresample=44100,aformat=sample_fmts=s16:channel_layouts=mono";
        const enum AVSampleFormat out_sample_fmts[] = { AV_SAMPLE_FMT_S16, AV_SAMPLE_FMT_NONE };
        const int out_sample_rates[] = { 44100, -1 };
        const AVFilterLink *outlink;

        int ret = 0;
    
        reader = new CoreMedia::MediaReader(url);
        ret = reader->prepare();
        client_print_message3("AAAA: [Test][MediaReader] prepared with status: %d", ret);
        if ( ret < 0 ) {
            goto end;
        }
        
        stream_idx = reader->findBestStream(AVMEDIA_TYPE_AUDIO);
        if ( stream_idx < 0 ) {
            client_print_message3("AAAA: [Test][MediaReader] Could not find audio stream");
            goto end;
        }
    
        decoder = new CoreMedia::MediaDecoder();
        ret = decoder->prepare(reader->getBestStream(AVMEDIA_TYPE_AUDIO));
        client_print_message3("AAAA: [Test][MediaDecoder] prepared with status: %d", ret);
        if ( ret < 0 ) {
            goto end;
        }
    
        filter_graph = avfilter_graph_alloc();
        if ( filter_graph == nullptr ) {
            client_print_message3("AAAA: [Test][FilterGraph] Could not allocate 'filter_graph'.");
            goto end;
        }
    
        ret = avfilter_graph_create_filter(&buffersrc_ctx, abuffersrc, "in", decoder->makeSrcArgs().c_str(), NULL, filter_graph);
        if (ret < 0) {
            client_print_message3("AAAA: [Test][FilterGraph] Cannot create buffer source");
            goto end;
        }
    
        ret = avfilter_graph_create_filter(&buffersink_ctx, abuffersink, "out", NULL, NULL, filter_graph);
        if (ret < 0) {
            client_print_message3("AAAA: [Test][FilterGraph] Cannot create audio buffer sink");
            goto end;
        }
    
        ret = av_opt_set_int_list(buffersink_ctx, "sample_fmts", out_sample_fmts, -1, AV_OPT_SEARCH_CHILDREN);
        if (ret < 0) {
            client_print_message3("AAAA: [Test][FilterGraph] Cannot set output sample format");
            goto end;
        }
    
        ret = av_opt_set(buffersink_ctx, "ch_layouts", "mono", AV_OPT_SEARCH_CHILDREN);
        if (ret < 0) {
            client_print_message3("AAAA: [Test][FilterGraph] Cannot set output channel layout");
            goto end;
        }
    
        ret = av_opt_set_int_list(buffersink_ctx, "sample_rates", out_sample_rates, -1, AV_OPT_SEARCH_CHILDREN);
        if (ret < 0) {
            client_print_message3("AAAA: [Test][FilterGraph] Cannot set output sample rate");
            goto end;
        }
    
        outputs = avfilter_inout_alloc();
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
    
        inputs = avfilter_inout_alloc();
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
            client_print_message3("AAAA: [Test][FilterGraph] filter graph parse failed with status: %d", ret);
            goto end;
        }

        if ( (ret = avfilter_graph_config(filter_graph, NULL)) < 0 ) {
            client_print_message3("AAAA: [Test][FilterGraph] filter graph config failed with status: %d", ret);
            goto end;
        }
    
        // 检查输入和输出
        if (inputs) {
            std::string output = "AAAA: [Test][FilterGraph] Input labels: ";
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
            std::string output = "AAAA: [Test][FilterGraph] Output labels: ";
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
        client_print_message3("AAAA: [Test][FilterGraph] Output: srate:%dHz fmt:%s chlayout:%s\n", (int)outlink->sample_rate, (char *)av_x_if_null(av_get_sample_fmt_name(static_cast<AVSampleFormat>(outlink->format)), "?"), args);
    
        avfilter_inout_free(&inputs);
        avfilter_inout_free(&outputs);
        
        pkt = av_packet_alloc();
        frame = av_frame_alloc();
        filt_frame = av_frame_alloc();
        while (1) {
            ret = decoder->receive(frame);  // receive frame
            if ( ret >= 0 ) {
                client_print_message3("AAAA: [Test][FilterGraph] decode frame success: nb_samples=%d, sample_rate=%d, nb_channels=%d, format=%d", frame->nb_samples, frame->sample_rate, frame->ch_layout.nb_channels, frame->format);
                
                // filter
                /* push the audio data from decoded frame into the filtergraph */
                if (av_buffersrc_add_frame_flags(buffersrc_ctx, frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
                    client_print_message3("AAAA: [Test][FilterGraph] Error while feeding the audio filtergraph\n");
                    break;
                }
                /* pull filtered audio from the filtergraph */
                while (1) {
                    ret = av_buffersink_get_frame(buffersink_ctx, filt_frame);
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                        break;
                    if (ret < 0)
                        goto end;
                    client_print_message3("AAAA: [Test][FilterGraph] filter frame success: nb_samples=%d, sample_rate=%d, nb_channels=%d, format=%d", filt_frame->nb_samples, filt_frame->sample_rate, filt_frame->ch_layout.nb_channels, filt_frame->format);
                    av_frame_unref(filt_frame);
                }
            
                av_frame_unref(frame);
            }
            else if ( ret == AVERROR(EAGAIN) ) {
                ret = reader->readPacket(pkt); // read pkt
                if ( ret < 0 ) {
                    break;
                }
                
                if ( pkt->stream_index == stream_idx ) {
                    ret = decoder->send(pkt); // send pkt
                    if ( ret < 0 ) {
                        break;
                    }
                }
                av_packet_unref(pkt);
            }
            else {
                break;
            }
        }
        
    end: 
        client_print_message3("AAAA: [Test] end");
        if ( pkt != nullptr ) av_packet_free(&pkt);
        if ( frame != nullptr ) av_frame_free(&frame);
        if ( decoder != nullptr ) delete decoder;
        if ( reader != nullptr ) delete reader;
        if ( inputs != nullptr ) avfilter_inout_free(&inputs);
        if ( outputs != nullptr ) avfilter_inout_free(&outputs);
        if ( filt_frame != nullptr ) av_frame_free(&filt_frame);
        if ( filter_graph != nullptr ) avfilter_graph_free(&filter_graph);
    }

    void test(const std::string& url) {
        testMediaDecoder2(url);
//         testFilterGraph(url);
    }
}