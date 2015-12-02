#pragma once

#ifndef CODEC_UTILS_H
#define CODEC_UTILS_H


extern "C" {
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

class CodecUtils
{
public:
    typedef struct {
        int codec_fmt;

        int src_width;
        int src_height;
        uint8_t* src_buf;
        int src_buf_size;

        //int dst_fmt; should be decided by codec
        int dst_width;
        int dst_height;
        uint8_t* dst_buf;
        int* dst_buf_size;
    } CODEC_INFO;
public:
    static void Init(AVCodecID codec_id);

    static void codec_log_callback(
        void *ptr,
        int level,
        const char *fmt,
        va_list vargs);

    static int img_convert(
        AVPicture *dst, int dst_pix_fmt,
        const AVPicture *src, int src_pix_fmt,
        int src_width, int src_height,
        int des_width, int des_height);

    static void video_encode(const CODEC_INFO& info);

    static void video_decode(const CODEC_INFO& info);
private:
    static void print(const char *fmt, va_list vargs, char* szBuf);
private:
    static bool sm_bInit;
    static AVCodecID sm_codecID;
    static CCriticalSection sm_cs;
};

#endif//CODEC_UTILS_H