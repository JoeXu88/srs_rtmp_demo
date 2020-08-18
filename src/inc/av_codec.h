#pragma once

#include <string>
#include <vector>
#include <queue>

extern "C" {
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
#include <libavutil/error.h>
#include "libavcodec/avcodec.h"
}

static const int FRAME_MAX_SIZE = 2;

static inline std::string ff_av_err2str(int errnum)
{
    char buf[AV_ERROR_MAX_STRING_SIZE] = { 0 };
    return std::string(av_make_error_string(buf, AV_ERROR_MAX_STRING_SIZE, errnum));
}

struct avfmt
{
    //audio fmt
    int channel;
    int bit_depth;
    int sample_rate;

    //TODO video fmt

    avfmt()
    {
        channel = -1;
        bit_depth = -1;
        sample_rate = -1;
    }
};

class AVDecoder
{
private:
    AVCodecContext* avcdec_ctx_;
    AVPacket* pkt_;
    AVCodecID codec_id_;
    AVMediaType media_type_;
    std::queue<AVFrame*> frames_dec_;
    std::queue<AVFrame*> frames_ready_;
    int max_frame_que_;
    bool inited_;

public:
    AVDecoder(const AVCodecID id, const AVMediaType type, const int max_frame)
    {
        codec_id_ = id;
        media_type_ = type;
        max_frame_que_ = max_frame > FRAME_MAX_SIZE ? max_frame : FRAME_MAX_SIZE;
        avcdec_ctx_ = NULL;
        pkt_ = NULL;
        inited_ = false;
    }

    ~AVDecoder()
    {
        uninit();
    }


    bool init(uint8_t* extra_data, const int extra_size, avfmt* fmt = NULL)
    {
        if (inited_) uninit();

        //avcodec_register_all();

        AVCodec*  dec = avcodec_find_decoder(codec_id_);
        if (!dec)
        {
            printf("can not find codec for avc\n");
            uninit();
            return false;
        }

        avcdec_ctx_ = avcodec_alloc_context3(dec);
        if (!avcdec_ctx_)
        {
            printf("can not alloc codec contex for avc\n");
            uninit();
            return false;
        }

        int ret = 0;

        //we must do this before open dec contex
        if (extra_data && extra_size > 0)
        {
            AVCodecParameters* codecpar = avcodec_parameters_alloc();
            if (!codecpar)
            {
                printf("can not alloc param\n");
                uninit();
                return false;
            }

            codecpar->codec_type = media_type_;
            codecpar->extradata = extra_data;
            codecpar->extradata_size = extra_size;
            ret = avcodec_parameters_to_context(avcdec_ctx_, codecpar);
            codecpar->extradata = NULL;
            codecpar->extradata_size = 0;
            avcodec_parameters_free(&codecpar);

            if (ret < 0) {
                printf("can not set param to codec avc\n");
                uninit();
                return false;
            }
        }

        if (media_type_ == AVMEDIA_TYPE_AUDIO)
        {
            avcdec_ctx_->channels = fmt->channel>0? fmt->channel:1;
            avcdec_ctx_->sample_rate = fmt->sample_rate>0? fmt->sample_rate:8000;
            avcdec_ctx_->sample_fmt = fmt->bit_depth>0? (AVSampleFormat)fmt->bit_depth:AV_SAMPLE_FMT_S16;
        }

        AVDictionary* opts = NULL;
        if ((ret = avcodec_open2(avcdec_ctx_, dec, &opts)) < 0) {
            printf("can not open avc codec\n");
            uninit();
            return false;
        }

        for (int i = 0; i < max_frame_que_; i++)
        {
            AVFrame* frame = av_frame_alloc();
            if (!frame) {
                fprintf(stderr, "Could not allocate frame\n");
                uninit();
                return false;
            }

            frames_dec_.push(frame);
        }

        pkt_ = av_packet_alloc();
        if (!pkt_)
        {
            fprintf(stderr, "Could not allocate pkt_\n");
            uninit();
            return false;
        }

        inited_ = true;
        return true;
    }

    void uninit()
    {
        if (avcdec_ctx_) avcodec_free_context(&avcdec_ctx_);
        if (pkt_) av_packet_free(&pkt_);
        for (int i = 0; i < frames_dec_.size(); i++)
        while(!frames_dec_.empty())
        {
            AVFrame* frame = frames_dec_.front();
            frames_dec_.pop();
            if (frame)
            {
                av_frame_unref(frame);
                av_frame_free(&frame);
            }
        }

        while (!frames_ready_.empty())
        {
            AVFrame* frame = frames_ready_.front();
            frames_ready_.pop();
            if (frame)
            {
                av_frame_unref(frame);
                av_frame_free(&frame);
            }
        }

        inited_ = false;
    }

    bool decode_frame(uint8_t* data, int size)
    {
        if (!inited_)
        {
            fprintf(stderr, "avc dec not inited_ yet\n");
            return false;
        }

        if (frames_dec_.empty())
        {
            printf("no free frame\n");
            return false;
        }

        //not thread safe
        AVFrame* frame = frames_dec_.front();
        av_frame_unref(frame);
        av_packet_unref(pkt_);
        pkt_->data = data;
        pkt_->size = size;

        int ret = avcodec_send_packet(avcdec_ctx_, pkt_);
        if (ret < 0) {
            fprintf(stderr, "Error submitting a packet for decoding (%s)\n", ff_av_err2str(ret).c_str());
            return false;
        }

        while (ret >= 0)
        {
            ret = avcodec_receive_frame(avcdec_ctx_, frame);
            if (ret < 0) {
                // those two return values are special and mean there is no output
                // frame available, but there were no errors during decoding
                if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
                {
                    printf("decode finish\n");
                    return true;
                }

                av_frame_unref(frame);
                fprintf(stderr, "Error during decoding (%s)\n", ff_av_err2str(ret).c_str());
                return false;
            }

            //push to ready list
            frames_dec_.pop();
            frames_ready_.push(frame);
            if (frames_dec_.empty()) break;
            else frame = frames_dec_.front();
        }

        return true;
    }

    AVFrame* getDecodedFrame()
    {
        if (!frames_ready_.empty())
        {
            AVFrame* frame = frames_ready_.front();
            frames_ready_.pop();
            frames_dec_.push(frame);

            return frame;
        }
        else
        {
            return NULL;
        }
    }
};


class H264Decoer : public AVDecoder
{
//private:
public:
    H264Decoer() :AVDecoder(AV_CODEC_ID_H264, AVMEDIA_TYPE_VIDEO, FRAME_MAX_SIZE) {}
    ~H264Decoer() {}
};

class AACDecoder : public AVDecoder
{
public:
    AACDecoder() :AVDecoder(AV_CODEC_ID_AAC, AVMEDIA_TYPE_AUDIO, FRAME_MAX_SIZE) {}
    ~AACDecoder() {}
};