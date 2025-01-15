//
// Created on 2025/1/8.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "Test.h"
#include "MediaReader.h"
#include "MediaDecoder.h"
#include "av/core/AudioFifo.h"
#include "av/core/AudioRenderer.h"
#include "av/core/FilterGraph.h"
#include "extension/client_print.h"
#include <cstddef>
#include <cstdint>
#include <thread>
#include <string>
#include <sstream>

extern "C" {
#include <libavutil/opt.h>
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavutil/pixdesc.h"
}

namespace CoreMedia {
    void testMediaReader(const std::string& url) {
        client_print_message3("AAAA: [Test] url=%s", url.c_str());
    
        CoreMedia::MediaReader* reader = new CoreMedia::MediaReader(url);
        int ret = reader->open();
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
        ret = reader->open();
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
        ret = decoder->init(reader->getStream(stream_idx)->codecpar);
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
        ret = reader->open();
        client_print_message3("AAAA: [Test][MediaReader] prepared with status: %d %s", ret, av_err2str(ret));
        if ( ret < 0 ) {
            goto end;
        } 
    
        stream_idx = reader->findBestStream(AVMEDIA_TYPE_AUDIO);
        if ( stream_idx == AVERROR_STREAM_NOT_FOUND ) {
            client_print_message3("AAAA: [Test][MediaReader] Could not find audio stream");
            goto end;
        }
    
        decoder = new CoreMedia::MediaDecoder();
        ret = decoder->init(reader->getStream(stream_idx)->codecpar);
        client_print_message3("AAAA: [Test][MediaDecoder] prepared with status: %d %s", ret, av_err2str(ret));
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
                    client_print_message3("AAAA: [Test][MediaDecoder] receive status: %d %s", ret, av_err2str(ret));
                
                    if ( ret >= 0 ) {
                        client_print_message3("AAAA: [Test][MediaDecoder] decode frame success: nb_samples=%d, sample_rate=%d, nb_channels=%d, format=%d", frame->nb_samples, frame->sample_rate, frame->ch_layout.nb_channels, frame->format);
                        av_frame_unref(frame);
                    }
                    else if ( ret == AVERROR(EAGAIN) ) {
                        ret = reader->readPacket(pkt); // read pkt
                        client_print_message3("AAAA: [Test][MediaReader] readPacket status: %d %s", ret, av_err2str(ret));
                        
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
                            client_print_message3("AAAA: [Test][MediaDecoder] send status: %d %s", ret, av_err2str(ret));
                        
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

    void testFilterGraph(const std::string& url) {
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
        const AVStream *astream = nullptr;
    
        AVBufferSrcParameters *abuffersrc_params = nullptr;
        const char *filter_descr = "aresample=44100,aformat=sample_fmts=s16:channel_layouts=mono";
        const enum AVSampleFormat out_sample_fmts[] = { AV_SAMPLE_FMT_S16, AV_SAMPLE_FMT_NONE };
        const int out_sample_rates[] = { 44100, -1 };
        const AVFilterLink *outlink;

        int ret = 0;
    
        reader = new CoreMedia::MediaReader(url);
        ret = reader->open();
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
        astream = reader->getStream(stream_idx);
        ret = decoder->init(astream->codecpar);
        client_print_message3("AAAA: [Test][MediaDecoder] prepared with status: %d", ret);
        if ( ret < 0 ) {
            goto end;
        }
    
        filter_graph = avfilter_graph_alloc();
        if ( filter_graph == nullptr ) {
            client_print_message3("AAAA: [Test][FilterGraph] Could not allocate 'filter_graph'.");
            goto end;
        }
    
        buffersink_ctx = avfilter_graph_alloc_filter(filter_graph, abuffersrc, "in");
        abuffersrc_params = decoder->createBufferSrcParameters(astream->time_base);
        ret = av_buffersrc_parameters_set(buffersink_ctx, abuffersrc_params);
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
    
        client_print_message3("AAAA: [Test][FilterGraph] filter graph parse before: inputs=%p, outputs=%p", inputs, outputs);
        ret = avfilter_graph_parse_ptr(filter_graph, filter_descr, &inputs, &outputs, NULL);
        client_print_message3("AAAA: [Test][FilterGraph] filter graph parsed with status: %d, inputs=%p, outputs=%p", ret, inputs, outputs);
    
        if ( ret < 0 ) {
            goto end;
        }

        // 检查输入和输出
        if (inputs != nullptr) {
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
    
        if (outputs != nullptr) {
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
    
    
        if ( (ret = avfilter_graph_config(filter_graph, NULL)) < 0 ) {
            client_print_message3("AAAA: [Test][FilterGraph] filter graph config failed with status: %d", ret);
            goto end;
        }
    
        if ( inputs != nullptr ) avfilter_inout_free(&inputs);
        if ( outputs != nullptr ) avfilter_inout_free(&outputs);
        
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
        if ( abuffersrc_params != nullptr ) av_free(abuffersrc_params);
    }


    std::string makeAudioBufferSourceArgs(AVBufferSrcParameters* params) {
        AVRational time_base = params->time_base;
        int sample_rate = params->sample_rate;
        AVSampleFormat sample_fmt = static_cast<AVSampleFormat>(params->format);
        AVChannelLayout ch_layout = params->ch_layout;
    
        // get channel layout desc
        char ch_layout_desc[64];
        av_channel_layout_describe(&ch_layout, ch_layout_desc, sizeof(ch_layout_desc));
        
        std::stringstream src_ss;
        src_ss  << "time_base=" << time_base.num << "/" << time_base.den
                << ":sample_rate=" << sample_rate
                << ":sample_fmt=" << av_get_sample_fmt_name(sample_fmt)
                << ":channel_layout=" << ch_layout_desc;
        return src_ss.str();
    }

    std::string makeVideoBufferSourceArgs(AVBufferSrcParameters* params) {
        AVRational time_base = params->time_base;
        int width = params->width;
        int height = params->height; 
        AVPixelFormat pix_fmt = static_cast<AVPixelFormat>(params->format);
        AVRational sar = params->sample_aspect_ratio;
        AVRational frame_rate = params->frame_rate;
    
        std::stringstream src_ss;
        src_ss  << "video_size=" << width << "x" << height
                << ":pix_fmt=" << pix_fmt
                << ":time_base=" << time_base.num << "/" << time_base.den
                << ":pixel_aspect=" << sar.num << "/" << sar.den;
        if ( frame_rate.num ) {
            src_ss << ":frame_rate=" << frame_rate.num << "/" << frame_rate.den;
        }
        return src_ss.str();
    }


    void testFilterGraph2(const std::string& url) {
        client_print_message3("AAAA: [Test] url=%s", url.c_str());
    
        CoreMedia::MediaReader* reader = nullptr;
        CoreMedia::MediaDecoder* audioDecoder = nullptr;
        CoreMedia::MediaDecoder* videoDecoder = nullptr;
        int audio_stream_idx = -1;
        int video_stream_idx = -1;
        const AVStream* astream = nullptr;
        const AVStream* vstream = nullptr;
        AVPacket *pkt = nullptr;
        AVFrame *frame = nullptr;
    
        AVFilterGraph *filter_graph = nullptr;
        
        const AVFilter *vbuffersrc = avfilter_get_by_name("buffer");
        const AVFilter *vbuffersink = avfilter_get_by_name("buffersink");
        const AVFilter *abuffersrc  = avfilter_get_by_name("abuffer");
        const AVFilter *abuffersink = avfilter_get_by_name("abuffersink");
        
        AVFilterContext *vbuffersink_ctx = nullptr;
        AVFilterContext *vbuffersrc_ctx = nullptr;
        AVFilterContext *abuffersink_ctx = nullptr;
        AVFilterContext *abuffersrc_ctx = nullptr;
    
        AVBufferSrcParameters* a_src_params = nullptr;
        AVBufferSrcParameters* v_src_params = nullptr;
    
        AVFilterInOut *outputs = nullptr;
        AVFilterInOut *inputs  = nullptr;
        AVFrame *filt_frame = nullptr;
    
        const AVFilterLink *outlink;
    
        const char *filter_descr =  
                                    "[0:a]aresample=44100,aformat=sample_fmts=s16:channel_layouts=mono[outa];"
                                    "[0:v]scale=320:-1[outv]"
    ;
    
        const AVSampleFormat out_sample_fmts[] = { AV_SAMPLE_FMT_S16, AV_SAMPLE_FMT_NONE };
        const int out_sample_rates[] = { 44100, -1 };
        const AVPixelFormat out_pix_fmts[] = { AV_PIX_FMT_GRAY8, AV_PIX_FMT_NONE };

        int ret = 0;
    
        reader = new CoreMedia::MediaReader(url);
        ret = reader->open();
        client_print_message3("AAAA: [Test][MediaReader] prepared with status: %d", ret);
        if ( ret < 0 ) {
            goto end;
        }
        
        audio_stream_idx = reader->findBestStream(AVMEDIA_TYPE_AUDIO);
        if ( audio_stream_idx < 0 ) {
            client_print_message3("AAAA: [Test][MediaReader] Could not find audio stream");
            goto end;
        }
    
        video_stream_idx = reader->findBestStream(AVMEDIA_TYPE_VIDEO);
        if ( video_stream_idx < 0 ) {
            client_print_message3("AAAA: [Test][MediaReader] Could not find video stream");
            goto end;
        }
    
        audioDecoder = new CoreMedia::MediaDecoder();
        astream = reader->getStream(audio_stream_idx);
        ret = audioDecoder->init(astream->codecpar);
        client_print_message3("AAAA: [Test][MediaDecoder] audioDecoder prepared with status: %d", ret);
        if ( ret < 0 ) {
            goto end;
        }
    
        videoDecoder = new CoreMedia::MediaDecoder();
        vstream = reader->getStream(video_stream_idx);
        ret = videoDecoder->init(vstream->codecpar);
        client_print_message3("AAAA: [Test][MediaDecoder] videoDecoder prepared with status: %d", ret);
        if ( ret < 0 ) {
            goto end;
        }
    
        filter_graph = avfilter_graph_alloc();
        if ( filter_graph == nullptr ) {
            client_print_message3("AAAA: [Test][FilterGraph] Could not allocate 'filter_graph'.");
            goto end;
        }
    
        a_src_params = audioDecoder->createBufferSrcParameters(astream->time_base);
        client_print_message3("AAAA: [Test] audio src args=%s", makeAudioBufferSourceArgs(a_src_params).c_str());
        ret = avfilter_graph_create_filter(&abuffersrc_ctx, abuffersrc, "0:a", makeAudioBufferSourceArgs(a_src_params).c_str(), NULL, filter_graph);
        if (ret < 0) {
            client_print_message3("AAAA: [Test][FilterGraph] Cannot create abuffer source");
            goto end;
        }
    
        ret = avfilter_graph_create_filter(&abuffersink_ctx, abuffersink, "outa", NULL, NULL, filter_graph);
        if (ret < 0) {
            client_print_message3("AAAA: [Test][FilterGraph] Cannot create audio buffer sink");
            goto end;
        }
    
        ret = av_opt_set_int_list(abuffersink_ctx, "sample_fmts", out_sample_fmts, -1, AV_OPT_SEARCH_CHILDREN);
        if (ret < 0) {
            client_print_message3("AAAA: [Test][FilterGraph] Cannot set output sample format");
            goto end;
        }
    
        ret = av_opt_set(abuffersink_ctx, "ch_layouts", "mono", AV_OPT_SEARCH_CHILDREN);
        if (ret < 0) {
            client_print_message3("AAAA: [Test][FilterGraph] Cannot set output channel layout");
            goto end;
        }
    
        ret = av_opt_set_int_list(abuffersink_ctx, "sample_rates", out_sample_rates, -1, AV_OPT_SEARCH_CHILDREN);
        if (ret < 0) {
            client_print_message3("AAAA: [Test][FilterGraph] Cannot set output sample rate");
            goto end;
        }
    
        v_src_params = videoDecoder->createBufferSrcParameters(vstream->time_base);
        client_print_message3("AAAA: [Test] video src args=%s", makeVideoBufferSourceArgs(v_src_params).c_str());
        ret = avfilter_graph_create_filter(&vbuffersrc_ctx, vbuffersrc, "0:v", makeVideoBufferSourceArgs(v_src_params).c_str(), NULL, filter_graph);
        if (ret < 0) {
            client_print_message3("AAAA: [Test][FilterGraph] Cannot create vbuffer source");
            goto end;
        }
      
        ret = avfilter_graph_create_filter(&vbuffersink_ctx, vbuffersink, "outv", NULL, NULL, filter_graph);
        if (ret < 0) {
            client_print_message3("AAAA: [Test][FilterGraph] Cannot create video buffer sink");
            goto end;
        }
    
        ret = av_opt_set_int_list(vbuffersink_ctx, "pix_fmts", out_pix_fmts, AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
        if (ret < 0) {
            client_print_message3("AAAA: [Test][FilterGraph] Cannot set output pixel format");
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
        outputs->name       = av_strdup("0:a");
        outputs->filter_ctx = abuffersrc_ctx;
        outputs->pad_idx    = 0;
        outputs->next       = avfilter_inout_alloc();
        outputs->next->name         = av_strdup("0:v");
        outputs->next->filter_ctx   = vbuffersrc_ctx;
        outputs->next->pad_idx      = 0;
        outputs->next->next         = NULL;
    
        inputs = avfilter_inout_alloc();
        /*
         * The buffer sink input must be connected to the output pad of
         * the last filter described by filters_descr; since the last
         * filter output label is not specified, it is set to "out" by
         * default.
         */
        inputs->name       = av_strdup("outa");
        inputs->filter_ctx = abuffersink_ctx;
        inputs->pad_idx    = 0;
        inputs->next       = avfilter_inout_alloc();    
        inputs->next->name          = av_strdup("outv");
        inputs->next->filter_ctx    = vbuffersink_ctx;
        inputs->next->pad_idx       = 0;
        inputs->next->next          = NULL;
    
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
        outlink = abuffersink_ctx->inputs[0];
        char args[512];
        av_channel_layout_describe(&outlink->ch_layout, args, sizeof(args));
        client_print_message3("AAAA: [Test][FilterGraph] Audio output: srate:%dHz fmt:%s chlayout:%s\n", (int)outlink->sample_rate, (char *)av_x_if_null(av_get_sample_fmt_name(static_cast<AVSampleFormat>(outlink->format)), "?"), args);
        outlink = vbuffersink_ctx->inputs[0]; 
        client_print_message3("AAAA: [Test][FilterGraph] Video output: w:%d h:%d sar:%d/%d fmt:%d\n", outlink->w, outlink->h, outlink->sample_aspect_ratio.num/outlink->sample_aspect_ratio.den, outlink->format);
    
        if ( inputs != nullptr ) avfilter_inout_free(&inputs);
        if ( outputs != nullptr ) avfilter_inout_free(&outputs);
        
        pkt = av_packet_alloc();
        frame = av_frame_alloc();
        filt_frame = av_frame_alloc();
        while (1) {
            ret = audioDecoder->receive(frame);  // receive audio frame
            client_print_message3("AAAA: [Test][MediaDecoder] receive audio frame status: %d %s", ret, av_err2str(ret));
            if ( ret >= 0 ) {
                client_print_message3("AAAA: [Test][FilterGraph] receive audio frame success: nb_samples=%d, sample_rate=%d, nb_channels=%d, format=%d", frame->nb_samples, frame->sample_rate, frame->ch_layout.nb_channels, frame->format);

                // filter
                /* push the audio data from decoded frame into the filtergraph */
                if (av_buffersrc_add_frame_flags(abuffersrc_ctx, frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
                    client_print_message3("AAAA: [Test][FilterGraph] Error while feeding the audio filtergraph\n");
                    break;
                }
                /* pull filtered audio from the filtergraph */
                while (1) {
                    ret = av_buffersink_get_frame(abuffersink_ctx, filt_frame);
                    client_print_message3("AAAA: [Test][FilterGraph] audio buffersink get frame status: %d %s", ret, av_err2str(ret));
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                        break;
                    if (ret < 0)
                        goto end;
                    client_print_message3("AAAA: [Test][FilterGraph] filter audio frame success: nb_samples=%d, sample_rate=%d, nb_channels=%d, format=%d", filt_frame->nb_samples, filt_frame->sample_rate, filt_frame->ch_layout.nb_channels, filt_frame->format);
                    av_frame_unref(filt_frame);
                }

                av_frame_unref(frame);
            }
            
            ret = videoDecoder->receive(frame);  // receive video frame
            client_print_message3("AAAA: [Test][MediaDecoder] receive video frame status: %d %s", ret, av_err2str(ret));
            if ( ret >= 0 ) {
                client_print_message3("AAAA: [Test][FilterGraph] receive video frame success: w=%d, h=%d, format=%d", frame->width, frame->height, frame->format);

                // filter
                /* push the video data from decoded frame into the filtergraph */
                if (av_buffersrc_add_frame_flags(vbuffersrc_ctx, frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
                    client_print_message3("AAAA: [Test][FilterGraph] Error while feeding the video filtergraph\n");
                    break;
                }
                /* pull filtered video from the filtergraph */
                while (1) {
                    ret = av_buffersink_get_frame(vbuffersink_ctx, filt_frame);
                    client_print_message3("AAAA: [Test][FilterGraph] video buffersink get frame status: %d %s", ret, av_err2str(ret));
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                        break;
                    if (ret < 0)
                        goto end;
                    client_print_message3("AAAA: [Test][FilterGraph] filter video frame success: w=%d, h=%d, format=%d", filt_frame->width, filt_frame->height, filt_frame->format);
                    av_frame_unref(filt_frame);
                }

                av_frame_unref(frame);
            }
            
            if ( ret == AVERROR(EAGAIN) ) {
                ret = reader->readPacket(pkt); // read pkt
                client_print_message3("AAAA: [Test][MediaReader] read pkt status: %d %s", ret, av_err2str(ret));
                if ( ret < 0 ) {
                    break;
                }

                if ( pkt->stream_index == audio_stream_idx ) {
                    ret = audioDecoder->send(pkt); // send pkt
                    client_print_message3("AAAA: [Test][MediaDecoder] send audio pkt status: %d %s", ret, av_err2str(ret));
                    if ( ret < 0 ) {
                        break;
                    }
                }
                else if ( pkt->stream_index == video_stream_idx ) {
                    ret = videoDecoder->send(pkt); // send pkt
                    client_print_message3("AAAA: [Test][MediaDecoder] send video pkt status: %d %s", ret, av_err2str(ret));
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
        if ( audioDecoder != nullptr ) delete audioDecoder;
        if ( reader != nullptr ) delete reader;
        if ( inputs != nullptr ) avfilter_inout_free(&inputs);
        if ( outputs != nullptr ) avfilter_inout_free(&outputs);
        if ( filt_frame != nullptr ) av_frame_free(&filt_frame);
        if ( filter_graph != nullptr ) avfilter_graph_free(&filter_graph);
        if ( a_src_params != nullptr ) av_free(a_src_params);
        if ( v_src_params != nullptr ) av_free(v_src_params);
    }

    int getSinkFrames(const std::string& stream_type, const std::string& sink_name, CoreMedia::FilterGraph* filter_graph) {
        AVFrame *filt_frame = av_frame_alloc();
        int ret = 0;
        while (ret >= 0) {
            ret = filter_graph->getFrame(sink_name, filt_frame);
            client_print_message3("AAAA: [Test][%s] get sink frame status: %d %s", stream_type.c_str(), ret, av_err2str(ret));   
            if ( ret < 0 ) {
                break;
            }
            av_frame_unref(filt_frame);
        }
        av_frame_free(&filt_frame);
        return ret;
    }

    int filter(const std::string& stream_type, const std::string& src_name, const std::string& sink_name, CoreMedia::FilterGraph* filter_graph, AVFrame *frame) {
        int ret = 0;
        while (ret >= 0) {
            int flags = frame != nullptr ? AV_BUFFERSRC_FLAG_KEEP_REF : AV_BUFFERSRC_FLAG_PUSH;
            ret = filter_graph->addFrame(src_name, frame, flags);
            client_print_message3("AAAA: [Test][%s] add src frame status: %d %s", stream_type.c_str(), ret, av_err2str(ret));
        
            if ( ret < 0 ) {
                break;
            }
            
            ret = getSinkFrames(stream_type, sink_name, filter_graph);
        }
        return ret;
    }

    int transcode(const std::string& stream_type, CoreMedia::MediaDecoder* decoder, AVPacket *pkt, const std::string& src_name, const std::string& sink_name, CoreMedia::FilterGraph* filter_graph) {
        int ret = 0;
        while (ret >= 0) {
            ret = decoder->send(pkt);
            client_print_message3("AAAA: [Test][%s] send pkt status: %d %s", stream_type.c_str(), ret, av_err2str(ret));
            if ( ret < 0 ) {
                break;
            }
            
            AVFrame *frame = av_frame_alloc();
            while (true) {
                ret = decoder->receive(frame);
                client_print_message3("AAAA: [Test][%s] receive frame status: %d %s", stream_type.c_str(), ret, av_err2str(ret));
                if ( ret < 0 && ret != AVERROR_EOF ) {
                    break;
                }
            
                ret = filter(stream_type, src_name, sink_name, filter_graph, ret != AVERROR_EOF ? frame : NULL);
                av_frame_unref(frame);
                if ( ret < 0 && ret != AVERROR(EAGAIN) ) {
                    break;
                }
            }
            av_frame_free(&frame);
        }
        return ret;
    }

    void testFilterGraph3(const std::string& url) {
        client_print_message3("AAAA: [Test] url=%s", url.c_str());
            
        CoreMedia::MediaReader* reader = nullptr;
        CoreMedia::MediaDecoder* audioDecoder = nullptr;
        CoreMedia::MediaDecoder* videoDecoder = nullptr;
        CoreMedia::FilterGraph* filterGraph = nullptr;
        AVBufferSrcParameters* a_src_params = nullptr;
        AVBufferSrcParameters* v_src_params = nullptr;
    
        int ret = 0;
        int audio_stream_idx = -1;
        int video_stream_idx = -1;
        const AVStream* astream = nullptr;
        const AVStream* vstream = nullptr;

        const char *filter_descr =  
                                    "[0:a]aresample=44100,aformat=sample_fmts=s16:channel_layouts=mono[outa];"
                                    "[0:v]scale=320:-1[outv]"
    ;
        
        const AVSampleFormat out_sample_fmts[] = { AV_SAMPLE_FMT_S16, AV_SAMPLE_FMT_NONE };
        const int out_sample_rates[] = { 44100, -1 };
        const AVPixelFormat out_pix_fmts[] = { AV_PIX_FMT_GRAY8, AV_PIX_FMT_NONE };
    
        AVPacket *pkt = nullptr;
        AVFrame *frame = nullptr;
        AVFrame *filt_frame = nullptr;

        // open file 
        reader = new CoreMedia::MediaReader(url);
        ret = reader->open();
        client_print_message3("AAAA: [Test][MediaReader] prepared with status: %d", ret);
        if ( ret < 0 ) {
            goto end;
        }
        
        audio_stream_idx = reader->findBestStream(AVMEDIA_TYPE_AUDIO);
        client_print_message3("AAAA: [Test][MediaReader] Found audio stream at index: %d", audio_stream_idx);
        if ( audio_stream_idx < 0 ) {
            goto end;
        }
    
        video_stream_idx = reader->findBestStream(AVMEDIA_TYPE_VIDEO);
        client_print_message3("AAAA: [Test][MediaReader] Found video stream at index: %d", video_stream_idx);
        if ( video_stream_idx < 0 ) {
            goto end;
        }
    
        // create decoder
        audioDecoder = new CoreMedia::MediaDecoder();
        astream = reader->getStream(audio_stream_idx);
        ret = audioDecoder->init(astream->codecpar);
        client_print_message3("AAAA: [Test][MediaDecoder] audioDecoder prepared with status: %d", ret);
        if ( ret < 0 ) {
            goto end;
        }
    
        videoDecoder = new CoreMedia::MediaDecoder();
        vstream = reader->getStream(video_stream_idx);
        ret = videoDecoder->init(vstream->codecpar);
        client_print_message3("AAAA: [Test][MediaDecoder] videoDecoder prepared with status: %d", ret);
        if ( ret < 0 ) {
            goto end;
        }
    
        // create filter graph
        filterGraph = new CoreMedia::FilterGraph();
        ret = filterGraph->init();
        client_print_message3("AAAA: [Test][FilterGraph] filterGraph prepared with status: %d", ret);
        if ( ret < 0 ) {
            goto end;
        }
    
        a_src_params = audioDecoder->createBufferSrcParameters(astream->time_base);
        client_print_message3("AAAA: [Test] audio src args=%s", makeAudioBufferSourceArgs(a_src_params).c_str());
        ret = filterGraph->addBufferSourceFilter("0:a", AVMEDIA_TYPE_AUDIO, a_src_params);
        if ( ret < 0 ) {
            client_print_message3("AAAA: [Test][FilterGraph] Cannot create abuffer source");
            goto end;
        }
    
        v_src_params = videoDecoder->createBufferSrcParameters(vstream->time_base);
        client_print_message3("AAAA: [Test] video src args=%s", makeVideoBufferSourceArgs(v_src_params).c_str());
        ret = filterGraph->addBufferSourceFilter("0:v", AVMEDIA_TYPE_VIDEO, v_src_params);
        if ( ret < 0 ) {
            client_print_message3("AAAA: [Test][FilterGraph] Cannot create vbuffer source");
            goto end;
        }
    
        ret = filterGraph->addAudioBufferSinkFilter("outa", out_sample_rates, out_sample_fmts, "mono");
        if ( ret < 0 ) {
            client_print_message3("AAAA: [Test][FilterGraph] Cannot create audio buffer sink");
            goto end;
        }
    
        ret = filterGraph->addVideoBufferSinkFilter("outv", out_pix_fmts);
        if (ret < 0) {
            client_print_message3("AAAA: [Test][FilterGraph] Cannot set output pixel format");
            goto end;
        }
    
        ret = filterGraph->parse(filter_descr);
        if ( ret < 0 ) {
            client_print_message3("AAAA: [Test][FilterGraph] filter graph parse failed with status: %d", ret);
            goto end;
        }
        
        ret = filterGraph->configure();
        if ( ret < 0 ) {
            client_print_message3("AAAA: [Test][FilterGraph] filter graph config failed with status: %d", ret);
            goto end;
        }
    
        // decode & filter
        frame = av_frame_alloc();
        pkt = av_packet_alloc();
        filt_frame = av_frame_alloc();

        while (ret >= 0 || ret == AVERROR(EAGAIN) ) {
            ret = reader->readPacket(pkt);
            client_print_message3("AAAA: [Test] read pkt status: %d %s", ret, av_err2str(ret));   
            if ( ret == AVERROR_EOF ) {
                client_print_message3("AAAA: [Test] EOF");   
                transcode("Audio", audioDecoder, NULL, "0:a", "outa", filterGraph); // eof
                transcode("Video", videoDecoder, NULL, "0:v", "outv", filterGraph); // eof
                break;
            }
        
            if ( ret < 0 ) break;
            
            // audio pkt
            if ( pkt->stream_index == audio_stream_idx ) {
                ret = transcode("Audio", audioDecoder, pkt, "0:a", "outa", filterGraph);
            }
            // video pkt
            else {
                ret = transcode("Video", videoDecoder, pkt, "0:v", "outv", filterGraph);
            }
            av_packet_unref(pkt);
        }
        
        client_print_message3("AAAA: [Test] end");
    
    end:
        if ( reader != nullptr ) delete reader;
        if ( audioDecoder != nullptr ) delete audioDecoder;
        if ( videoDecoder != nullptr ) delete videoDecoder;
        if ( filterGraph != nullptr ) delete filterGraph;
        if ( a_src_params != nullptr ) av_free(a_src_params);
        if ( v_src_params != nullptr ) av_free(v_src_params);
        if ( frame != nullptr ) av_frame_free(&frame);
        if ( pkt != nullptr ) av_packet_free(&pkt);
        if ( filt_frame != nullptr ) av_frame_free(&filt_frame);
    }

    static bool filt_eof = false;
    static std::mutex renderer_mutex;

    int filter_get_frame(AudioFifo* fifo, const std::string& sink_name, CoreMedia::FilterGraph* filter_graph) {
        int ret = 0;
        AVFrame* frame = av_frame_alloc();
        while(ret >= 0) {
            ret = filter_graph->getFrame(sink_name, frame);
        
            client_print_message3("AAAA: [Test] buffersink get frame status: %d %s, fmt=%s, sample_rate=%d, nb_channels=%d, nb_samples=%d, pkt_dts=%d", ret, av_err2str(ret), ret >= 0 ? av_get_sample_fmt_name((AVSampleFormat)frame->format) : "null", ret >= 0 ? frame->sample_rate : 0, ret >= 0 ? frame->ch_layout.nb_channels : 0, ret >= 0 ? frame->nb_samples : 0, ret >= 0 ? frame->pkt_dts : 0);

            if ( ret == AVERROR_EOF ) {
                std::lock_guard<std::mutex> lock(renderer_mutex);
                filt_eof = true;
                break;
            }
        
            if ( ret < 0 ) {
                break;
            }
            
            std::lock_guard<std::mutex> lock(renderer_mutex);
            ret = fifo->write((void**)frame->data, frame->nb_samples);
            client_print_message3("AAAA: [Test] fifo write frame status: %d %s", ret, av_err2str(ret));
            av_frame_unref(frame);
        }
        av_frame_free(&frame);
        return ret;
    }

    int filter_add_frame(AudioFifo* fifo, AVFrame *frame, const std::string& src_name, const std::string& sink_name, CoreMedia::FilterGraph* filter_graph) {
        int flags = frame != nullptr ? AV_BUFFERSRC_FLAG_KEEP_REF : AV_BUFFERSRC_FLAG_PUSH;
        int ret = filter_graph->addFrame(src_name, frame, flags);
        client_print_message3("AAAA: [Test] buffersrc add frame status: %d %s, fmt=%s, sample_rate=%d, nb_channels=%d, nb_samples=%d", ret, av_err2str(ret), frame != NULL ? av_get_sample_fmt_name((AVSampleFormat)frame->format) : "null", frame != NULL ? frame->sample_rate : 0, frame != NULL ? frame->ch_layout.nb_channels : 0, frame != NULL ? frame->nb_samples : 0);
        if ( ret < 0 ) {
            return ret;
        }

        return filter_get_frame(fifo, sink_name, filter_graph);
    }

    int decoder_receive_frame(AudioFifo* fifo, CoreMedia::MediaDecoder* decoder, const std::string& src_name, const std::string& sink_name, CoreMedia::FilterGraph* filter_graph) {
        int ret = 0;
        AVFrame *frame = av_frame_alloc();
        while (ret >= 0) {
            ret = decoder->receive(frame);
            client_print_message3("AAAA: [Test] receive dec frame status: %d %s", ret, av_err2str(ret));
            if ( ret == AVERROR_EOF ) {
                ret = filter_add_frame(fifo, NULL, src_name, sink_name, filter_graph);
                break;
            }
        
            if ( ret < 0 ) {
                break;
            }
            
            ret = filter_add_frame(fifo, frame, src_name, sink_name, filter_graph);
            av_frame_unref(frame);
        }
        av_frame_free(&frame);
        return ret;
    }

    int decoder_send_pkt(AudioFifo* fifo, CoreMedia::MediaDecoder* decoder, AVPacket *pkt, const std::string& src_name, const std::string& sink_name, CoreMedia::FilterGraph* filter_graph) {
        int ret = decoder->send(pkt);
        client_print_message3("AAAA: [Test] send pkt status: %d %s", ret, av_err2str(ret));
        if ( ret < 0 ) {
            return ret;
        }
    
        return decoder_receive_frame(fifo, decoder, src_name, sink_name, filter_graph);
    }

    void testRenderer(const std::string& url) {
        client_print_message3("AAAA: [Test] url=%s", url.c_str());
        
        CoreMedia::MediaReader* reader = nullptr;
        CoreMedia::MediaDecoder* audio_decoder = nullptr;
        CoreMedia::FilterGraph* filter_graph = nullptr;
        CoreMedia::AudioFifo* audio_fifo = nullptr;
        CoreMedia::AudioRenderer* audio_renderer = nullptr;
        
        AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
        int out_sample_rate = 48000;
        int out_nb_channels = 1;
        const char* out_channel_layout = "mono";
    
        std::stringstream filter_descr_ss;
        filter_descr_ss << "[0:a]"
                        << "aresample=" << out_sample_rate
                        << ",aformat=sample_fmts=" << av_get_sample_fmt_name(out_sample_fmt) << ":channel_layouts=" << out_channel_layout
                        << "[outa]";

        std::string filter_descr = filter_descr_ss.str();
        const AVSampleFormat out_sample_fmts[] = { out_sample_fmt, AV_SAMPLE_FMT_NONE };
        const int out_sample_rates[] = { out_sample_rate, -1 };
        OH_AudioStream_SampleFormat renderder_sample_fmt = AUDIOSTREAM_SAMPLE_S16LE;
        const int renderer_sample_rate = out_sample_rate;
    
        client_print_message3("AAAA: [Test] out_sample_fmt=%s, out_samle_rate=%d, out_nb_channels=%d, out_channel_layout=%s, filter_descr=%s", av_get_sample_fmt_name(out_sample_fmt), out_sample_rate, out_nb_channels, out_channel_layout, filter_descr.c_str());
        
        AVPacket *pkt = nullptr;
        AVFrame *frame = nullptr;
        AVFrame *filt_frame = nullptr;
        AVBufferSrcParameters* a_src_params = nullptr;
        bool write_eof = false;

        int ret = 0;
        int audio_stream_idx = -1;
        const AVStream* astream = nullptr;
        OH_AudioStream_Result renderer_ret = AUDIOSTREAM_SUCCESS;
    
    
        // open file 
        reader = new CoreMedia::MediaReader(url);
        ret = reader->open();
        client_print_message3("AAAA: [Test][MediaReader] prepared with status: %d", ret);
        if ( ret < 0 ) {
            goto end;
        }
        
        audio_stream_idx = reader->findBestStream(AVMEDIA_TYPE_AUDIO);
        client_print_message3("AAAA: [Test][MediaReader] Found audio stream at index: %d", audio_stream_idx);
        if ( audio_stream_idx < 0 ) {
            goto end;
        }

        // create decoder
        audio_decoder = new CoreMedia::MediaDecoder();
        astream = reader->getStream(audio_stream_idx);
        ret = audio_decoder->init(astream->codecpar);
        client_print_message3("AAAA: [Test][MediaDecoder] audioDecoder prepared with status: %d", ret);
        if ( ret < 0 ) {
            goto end;
        }
    
        // create filter graph
        filter_graph = new CoreMedia::FilterGraph();
        ret = filter_graph->init();
        client_print_message3("AAAA: [Test][FilterGraph] filterGraph prepared with status: %d", ret);
        if ( ret < 0 ) {
            goto end;
        }
    
        a_src_params = audio_decoder->createBufferSrcParameters(astream->time_base);
        client_print_message3("AAAA: [Test] audio src args=%s, nb_channels=%d", makeAudioBufferSourceArgs(a_src_params).c_str(), a_src_params->ch_layout.nb_channels);
        ret = filter_graph->addBufferSourceFilter("0:a", AVMEDIA_TYPE_AUDIO, a_src_params);
        if ( ret < 0 ) {
            client_print_message3("AAAA: [Test][FilterGraph] Cannot create abuffer source");
            goto end;
        }
    
        ret = filter_graph->addAudioBufferSinkFilter("outa", out_sample_rates, out_sample_fmts, out_channel_layout);
        if ( ret < 0 ) {
            client_print_message3("AAAA: [Test][FilterGraph] Cannot create audio buffer sink");
            goto end;
        }
    
        ret = filter_graph->parse(filter_descr);
        if ( ret < 0 ) {
            client_print_message3("AAAA: [Test][FilterGraph] filter graph parse failed with status: %d", ret);
            goto end;
        }
        
        ret = filter_graph->configure();
        if ( ret < 0 ) {
            client_print_message3("AAAA: [Test][FilterGraph] filter graph config failed with status: %d", ret);
            goto end;
        }    
    
        audio_fifo = new CoreMedia::AudioFifo();
        ret = audio_fifo->init(out_sample_fmt, out_nb_channels, 1);
        if ( ret < 0 ) {
            client_print_message3("AAAA: [Test][AudioFifo] AudioFifo init failed with status: %d", ret);
            goto end;
        }    
        
        audio_renderer = new CoreMedia::AudioRenderer();
        renderer_ret = audio_renderer->init(renderder_sample_fmt, out_nb_channels, renderer_sample_rate);
        if ( renderer_ret != AUDIOSTREAM_SUCCESS ) {
            client_print_message3("AAAA: [Test][AudioRenderer] AudioRenderer init failed with status: %d", renderer_ret);
            goto end;
        }
    
        audio_renderer->setWriteDataCallback([&](void* _Nullable user_data, void* _Nonnull data, int32_t size) {
            client_print_message3("AAAA: [Test] write data callback: %d, filt_eof=%d, write_eof=%d", size, filt_eof, write_eof);

            if ( write_eof ) {
                return AUDIO_DATA_CALLBACK_RESULT_INVALID;
            }

            std::lock_guard<std::mutex> lock(renderer_mutex);
            if ( audio_fifo->getSize() >= size || filt_eof ) {
                int ret = audio_fifo->read(&data, size); 
                client_print_message3("AAAA: [Test] fifo read status: %d, size=%d", ret, size);

                if (ret >= 0) {
                    if (filt_eof && ret < size) {
                        memset(static_cast<uint8_t*>(data) + ret, 0, size - ret);
                        write_eof = true;
                    }
                    return AUDIO_DATA_CALLBACK_RESULT_VALID;
                }
            }
            return AUDIO_DATA_CALLBACK_RESULT_INVALID;
        }, nullptr);

        renderer_ret = audio_renderer->play();
    
        if ( renderer_ret != AUDIOSTREAM_SUCCESS ) {
            client_print_message3("AAAA: [Test][AudioRenderer] AudioRenderer play failed with status: %d", renderer_ret);
            goto end;
        }
    
        // decode & filter & renderer
        pkt = av_packet_alloc();
        frame = av_frame_alloc();
        filt_frame = av_frame_alloc();
        
        while (ret >= 0 || ret == AVERROR(EAGAIN)) {
            // read
            ret = reader->readPacket(pkt);
            client_print_message3("AAAA: [Test] read pkt status: %d %s", ret, av_err2str(ret));   
        
            if ( ret == AVERROR_EOF ) {
                ret = decoder_send_pkt(audio_fifo, audio_decoder, NULL, "0:a", "outa", filter_graph);
                break;
            }
        
            if ( ret < 0 ) {
                break;
            }
        
            if ( pkt->stream_index == audio_stream_idx ) {
                ret = decoder_send_pkt(audio_fifo, audio_decoder, pkt, "0:a", "outa", filter_graph);
            }
            av_packet_unref(pkt);
        }
    
        {
            std::thread wait([&] {
                std::this_thread::sleep_for(std::chrono::seconds(12)); // 延迟 12 秒         
            });
        
            wait.join();
        }
    
        audio_renderer->stop();
        
        client_print_message3("AAAA: [Test] end");
    
    end:
        if ( reader != nullptr ) delete reader;
        if ( audio_decoder != nullptr ) delete audio_decoder;
        if ( filter_graph != nullptr ) delete filter_graph;
        if ( audio_fifo != nullptr ) delete audio_fifo;
        if ( audio_renderer != nullptr ) delete audio_renderer;
        if ( pkt != nullptr ) av_packet_free(&pkt);
        if ( frame != nullptr ) av_frame_free(&frame);
        if ( filt_frame != nullptr ) av_frame_free(&filt_frame);
        if ( a_src_params != nullptr ) av_free(a_src_params);
    }

    void test(const std::string& url) {
//         testMediaDecoder2(url);
//         testFilterGraph(url);
//         testFilterGraph3(url);
        testRenderer(url);
    }
}


//     param = av_buffersrc_parameters_alloc();
//    
//     // 设置音频参数
//     param->channel_layout = &channel_layout; // 指向通道布局的指针（假设已初始化）
//     param->sample_rate = 44100;
//     param->format = AV_SAMPLE_FMT_FLTP;      // 浮点格式
//     param->time_base = (AVRational){1, 44100};