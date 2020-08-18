/**
# Example to use srs-librtmp
# see: https://github.com/winlinvip/simple-rtmp-server/wiki/v2_CN_SrsLibrtmp
    gcc main.cpp srs_librtmp.cpp -g -O0 -lstdc++ -o output
*/
#include <windows.h>
#include <stdio.h>
#include <string>
#include "..\src\srs\srs_librtmp.h"
#include "SDLRender/YuvRender.h"
#include "../src/inc/av_codec.h"
#include "../src/inc/helper.h"

static const int FLV_VD_DATA_OFFSET = 5; //h264 video data offset in flv tag data
static const int FLV_AD_DATA_OFFSET = 2; //aac audio data offset in flv tag data
static const int DEBUG_FRAME_N = 15;

static void getAVFMT(char* data, int size, avfmt& fmt)
{
    static int audio_smp_rate[] = { 5500, 11025, 22050, 44100 };
    static int audio_channel[] = { 1, 2 };
    static int audio_samp_size[] = { AV_SAMPLE_FMT_U8, AV_SAMPLE_FMT_S16 };
    char rate = srs_utils_flv_audio_sound_rate(data, size);
    char chnl = srs_utils_flv_audio_sound_type(data, size);
    char bit_dep = srs_utils_flv_audio_sound_size(data, size);

    if (chnl >= 0) fmt.channel = audio_channel[chnl];
    if (rate >= 0) fmt.sample_rate = audio_smp_rate[rate];
    if (bit_dep >= 0) fmt.bit_depth = audio_samp_size[bit_dep];
}

int main(int argc, char** argv)
{
    printf("suck rtmp stream like rtmpdump\n");
    printf("ossrs/srs client librtmp library.\n");
    printf("version: %d.%d.%d\n", srs_version_major(), srs_version_minor(), srs_version_revision());
    
 /*   if (argc <= 1) {
        printf("Usage: %s <rtmp_url>\n"
            "   rtmp_url     RTMP stream url to play\n"
            "For example:\n"
            "   %s rtmp://127.0.0.1:1935/live/livestream\n"
            "   %s rtmp://ossrs.net:1935/live/livestream\n",
            argv[0], argv[0], argv[0]);
        exit(-1);
    }*/
	
    // startup socket for windows.
    WSADATA WSAData;
    if (WSAStartup(MAKEWORD(1, 1), &WSAData)) {
        printf("WSAStartup failed.\n");
        return -1;
    }

    H264Decoer avcdec;
    AACDecoder aacdec;
    YuvRender render;
    FileDumper aacdump("test.pcm");
    
    std::string url = "rtmp://172.22.215.19/live/test";
    srs_human_trace("rtmp url: %s", url.c_str());
    srs_rtmp_t rtmp = srs_rtmp_create(url.c_str());
    
    if (srs_rtmp_handshake(rtmp) != 0) {
        srs_human_trace("simple handshake failed.");
        goto rtmp_destroy;
    }
    srs_human_trace("simple handshake success");
    
    if (srs_rtmp_connect_app(rtmp) != 0) {
        srs_human_trace("connect vhost/app failed.");
        goto rtmp_destroy;
    }
    srs_human_trace("connect vhost/app success");
    
    if (srs_rtmp_play_stream(rtmp) != 0) {
        srs_human_trace("play stream failed.");
        goto rtmp_destroy;
    }
    srs_human_trace("play stream success");
    
    render.Init();
    for (;;) {
        int size;
        char type;
        char* data;
        u_int32_t timestamp, pts;
        
        if (srs_rtmp_read_packet(rtmp, &type, &timestamp, &data, &size) != 0) {
            goto rtmp_destroy;
        }
        if (srs_utils_parse_timestamp(timestamp, type, data, size, &pts) != 0) {
            goto rtmp_destroy;
        }
        srs_human_trace("got packet: type=%s, dts=%d, pts=%d, size=%d", 
            srs_human_flv_tag_type2string(type), timestamp, pts, size);

        static int cnt = 0;
        if (type == SRS_RTMP_TYPE_VIDEO && ((data[0] & 0x0f) == SrsVideoCodecIdAVC))//make sure is h264
        {
            if (data[1] == SrsVideoAvcFrameTraitSequenceHeader) //sequence header frame
            {
                avcdec.init((uint8_t*)(data + FLV_VD_DATA_OFFSET), size - FLV_VD_DATA_OFFSET);
            }
            else if (data[1] == SrsVideoAvcFrameTraitNALU) //naul frame
            {
                if (avcdec.decode_frame((uint8_t*)(data + FLV_VD_DATA_OFFSET), size - FLV_VD_DATA_OFFSET))
                {
                    AVFrame* frame = avcdec.getDecodedFrame();
                    if (frame && frame->width > 0 && frame->height > 0)
                    {
                        printf("got frame w:%d, h:%d, linesize: %d %d %d\n", frame->width, frame->height,
                            frame->linesize[0], frame->linesize[1], frame->linesize[2]);

                        //yuv_dump.dump((char**)frame->data, frame->linesize, frame->width, frame->height);

                        render.Update(frame->width, frame->height, frame->data, frame->linesize);
                        render.Render();
                        //cnt++;
                    }
                    else printf("no frames decoded\n");

                }
            }
        }

        if (type == SRS_RTMP_TYPE_AUDIO && (((data[0] & 0xf0) >> 4) == SrsAudioCodecIdAAC))
        {
            printf("got aac frames\n");
            if (data[1] == SrsAudioAacFrameTraitSequenceHeader) //sequence header frame
            {
                avfmt fmt;
                getAVFMT(&data[0], 1, fmt);
                printf("got audio fmt=>sr:%d, channel:%d, fmt:%d\n", fmt.sample_rate, fmt.channel, fmt.bit_depth);
                aacdec.init((uint8_t*)(data + FLV_AD_DATA_OFFSET), size - FLV_AD_DATA_OFFSET, &fmt);
            }
            else if (data[1] == SrsAudioAacFrameTraitRawData)
            {
                aacdec.decode_frame((uint8_t*)(data + FLV_AD_DATA_OFFSET), size - FLV_AD_DATA_OFFSET);
                AVFrame* frame = aacdec.getDecodedFrame();
                if (frame && frame->nb_samples > 0)
                {
                    printf("deocoded audio frame, size:%d, channel: %d, fmt:%d, sr:%d\n", frame->nb_samples, frame->channels, 
                        frame->format,frame->sample_rate);
                    //aacdump.dump((char*)frame->data[0], frame->nb_samples * frame->channels * 4);
                }
            }
        }
        
        free(data);
    }
    
rtmp_destroy:
    srs_rtmp_destroy(rtmp);

    // cleanup socket for windows.
    WSACleanup();
    
    return 0;
}

