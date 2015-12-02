#include "StdAfx.h"
#include "CodecUtils.h"



bool CodecUtils::sm_bInit = false;
AVCodecID CodecUtils::sm_codecID = AV_CODEC_ID_NONE;
CCriticalSection CodecUtils::sm_cs;

void CodecUtils::Init(AVCodecID codec_id)
{
    avcodec_register_all();
    //av_log_set_callback(my_log_callback);
    //av_log_set_level(AV_LOG_VERBOSE);
    sm_codecID = codec_id;
    sm_bInit = true;
}

void CodecUtils::codec_log_callback(
    void *ptr,
    int level,
    const char *fmt,
    va_list vargs) 
{
    static char szInfo[256] = {0};
    print(fmt, vargs, szInfo);
    DEBUG_INFO(szInfo);
}

void CodecUtils::print(
    const char *fmt,
    va_list vargs,
    char* szBuf)  
{
    char *cb;  
    char *szTmp;  
    char c;  
    int nTmp;  
    unsigned int uiTmp;  

    cb = szBuf;

    while(*fmt)  
    {  
        if (*fmt != '%')  
        {  
            *cb = *fmt;  
            cb++;  
            fmt++;  
            continue;  
        }  

        fmt++;  

        switch(*fmt)  
        {  
        case 'd':  
            nTmp = va_arg(vargs, int);  
            _itoa(nTmp, cb, 10);  
            while(*++cb) ;  
            break;  
        case 'x':  
            uiTmp = va_arg(vargs, unsigned int);  
            _itoa(uiTmp, cb, 16);  
            while(*++cb) ;  
            break;  
        case 's':  
            szTmp = va_arg(vargs, char *);  
            while(*szTmp)  
            {  
                *cb++ = *szTmp;  
                szTmp++;  
            }  
            break;  
        case 'c':  
            c = va_arg(vargs, char);  
            *cb++ = c;  
            break;  
        default:  
            *cb++ = '%';  
        }  
        fmt++;  
    }  
    *cb = 0;
}

int CodecUtils::img_convert(
    AVPicture *dst, int dst_pix_fmt,
    const AVPicture *src, int src_pix_fmt,
    int src_width, int src_height,
    int des_width, int des_height)
{
    SwsContext *pSwsCtx;

    pSwsCtx = sws_getContext(src_width,
        src_height,
        (AVPixelFormat)src_pix_fmt,
        des_width,
        des_height,
        (AVPixelFormat)dst_pix_fmt,
        SWS_BICUBIC,//SWS_POINT,//,
        NULL, NULL, NULL);
    if (!pSwsCtx)
        return -1;

    int ret = sws_scale(pSwsCtx, src->data, src->linesize,
        0, src_height, dst->data, dst->linesize);

    //free pSwsCtx
    sws_freeContext(pSwsCtx);

    return ret;
}

void CodecUtils::video_encode(const CODEC_INFO& info)
{
    sm_cs.Lock();

    ASSERT(sm_bInit);
    int codec_id = sm_codecID;

    AVFrame *enc_yuv_frame, *enc_rgb_frame;
    AVCodecContext *enc_ctx= NULL;

    const long lEncodeFrames = 1;

    AVCodec *avcodec;
    int i, ret, x, y, got_output;
    AVPacket pkt;

    /* find the mpeg1 video encoder */
    avcodec = avcodec_find_encoder((AVCodecID) codec_id);
    if (!avcodec) {
        fprintf(stderr, "Codec not found\n");
        DebugBreak();
        exit(1);
    }

    enc_ctx = avcodec_alloc_context3(avcodec);
    if (!enc_ctx) {
        fprintf(stderr, "Could not allocate video codec context\n");
        DebugBreak();
        exit(1);
    }

    /* put sample parameters */
    enc_ctx->bit_rate = VIDEO_BIT_RATE;
    /* resolution must be a multiple of two */
    enc_ctx->width = info.dst_width;//352;
    enc_ctx->height = info.dst_height;//288;
    /* frames per second */
    enc_ctx->time_base.num = 1;
    enc_ctx->time_base.den = FRAMES_PER_SECOND;
    enc_ctx->gop_size = 10; /* emit one intra frame every ten frames */
    enc_ctx->max_b_frames = 1;
    enc_ctx->pix_fmt = AV_PIX_FMT_YUV420P;

    if(codec_id == AV_CODEC_ID_H264)
        av_opt_set(enc_ctx->priv_data, "preset", "veryfast", 0);

    /* open it */
    if (avcodec_open2(enc_ctx, avcodec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        DebugBreak();
        exit(1);
    }

    // convert rgb to yuv
    enc_yuv_frame = avcodec_alloc_frame();
    if (!enc_yuv_frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        DebugBreak();
        exit(1);
    }
    enc_yuv_frame->format = enc_ctx->pix_fmt;
    enc_yuv_frame->width  = enc_ctx->width;
    enc_yuv_frame->height = enc_ctx->height;

    /* the image can be allocated by any means and av_image_alloc() is
    / * just the most convenient way if av_malloc() is to be used */
    ret = av_image_alloc(enc_yuv_frame->data, 
        enc_yuv_frame->linesize,
        enc_ctx->width,
        enc_ctx->height,
        enc_ctx->pix_fmt, 32);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate raw picture buffer\n");
        DebugBreak();
        exit(1);
    }

    enc_rgb_frame = avcodec_alloc_frame();
    if (!enc_rgb_frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        DebugBreak();
        exit(1);
    }

    //CamPkgQueue::element_type& pBuf = pQueue->at(0);

    enc_rgb_frame->format = info.codec_fmt;//AV_PIX_FMT_RGB24;
    enc_rgb_frame->width  = info.src_width;//pBuf->m_nWidth;
    enc_rgb_frame->height = info.src_height;//pBuf->m_nHeight;

    for (int i=0; i < lEncodeFrames; i++)
    {
        ret = avpicture_fill(
            (AVPicture *)enc_rgb_frame,
            info.src_buf,
            (AVPixelFormat) info.codec_fmt,
            info.src_width,
            info.src_height);
        if (ret < 0) {
            fprintf(stderr, "Could not fill in raw rgb picture buffer\n");
            DebugBreak();
            exit(1);
        }

        ret = img_convert(
            (AVPicture*)enc_yuv_frame,
            PIX_FMT_YUV420P,
            (AVPicture*)enc_rgb_frame,
            info.codec_fmt,//AV_PIX_FMT_RGB24,
            enc_rgb_frame->width,
            enc_rgb_frame->height,
            enc_ctx->width,
            enc_ctx->height);
        if (ret < 0) {
            fprintf(stderr, "Could not convert raw rgb picture to yuv\n");
            DebugBreak();
            exit(1);
        }

        // encode the image
        av_init_packet(&pkt);
        pkt.data = NULL;    // packet data will be allocated by the encoder
        pkt.size = 0;

        ret = avcodec_encode_video2(enc_ctx, &pkt, enc_yuv_frame, &got_output);
        if (ret < 0) {
            fprintf(stderr, "Error encoding frame\n");
            DebugBreak();
            exit(1);
        }

        if (got_output) {
            //printf("Write frame %3d (size=%5d)\n", i, pkt.size);
            av_free_packet(&pkt);
        }
    }

    /* get the delayed frames */
    for (got_output = 1; got_output; ) {
        fflush(stdout);

        ret = avcodec_encode_video2(enc_ctx, &pkt, NULL, &got_output);
        if (ret < 0) {
            fprintf(stderr, "Error encoding frame\n");
            DebugBreak();
            exit(1);
        }

        if (got_output) {
            ASSERT(*info.dst_buf_size >= pkt.size);
            memcpy(info.dst_buf, pkt.data, pkt.size);
            *info.dst_buf_size = pkt.size;
            av_free_packet(&pkt);
        }
    }

    av_freep(&enc_yuv_frame->data[0]);
    avcodec_free_frame(&enc_yuv_frame);

    avcodec_close(enc_ctx);
    av_free(enc_ctx);

    sm_cs.Unlock();
}

void CodecUtils::video_decode(const CODEC_INFO& info)
{
    sm_cs.Lock();

    ASSERT(sm_bInit);
    int codec_id = sm_codecID;

    uint8_t* pBufH264 = info.src_buf;
    int nSizeH264 = info.src_buf_size;
    uint8_t* pBufRGB24 = info.dst_buf;
    int nSizeRGB24 = *info.dst_buf_size;

    AVCodec *codec;
    AVCodecContext *dec_ctx= NULL;
    AVFrame *dec_yuv_frame, *dec_rgb_frame;

    AVPacket pkt;


    uint8_t* inbuf = (uint8_t*) av_malloc(nSizeH264 + FF_INPUT_BUFFER_PADDING_SIZE);
    memcpy(inbuf, pBufH264, nSizeH264);
    /* set end of buffer to 0 (this ensures that no overreading happens for damaged mpeg streams) */
    memset(inbuf + nSizeH264, 0, FF_INPUT_BUFFER_PADDING_SIZE);


    memset(pBufRGB24, 0, nSizeRGB24);

    av_init_packet(&pkt);
    pkt.data = inbuf;
    pkt.size = nSizeH264;

    codec = avcodec_find_decoder((AVCodecID)codec_id);
    if (!codec) {
        fprintf(stderr, "Codec not found\n");
        DebugBreak();
        exit(1);
    }

    dec_ctx = avcodec_alloc_context3(codec);
    if (!dec_ctx) {
        fprintf(stderr, "Could not allocate video codec context\n");
        DebugBreak();
        exit(1);
    }

    dec_ctx->time_base.num = 1;
    dec_ctx->time_base.den = FRAMES_PER_SECOND;
    dec_ctx->bit_rate = 0;
    dec_ctx->frame_number = 1;
    dec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
    dec_ctx->width = info.src_width;//352;
    dec_ctx->height = info.src_height;//288;
    dec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;

    //av_log_set_callback(CodecUtils::codec_log_callback);
    //av_log_set_level(AV_LOG_VERBOSE);

    if(codec_id == AV_CODEC_ID_H264)
        av_opt_set(dec_ctx->priv_data, "preset", "veryfast", 0);

    if (avcodec_open2(dec_ctx, codec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        DebugBreak();
        exit(1);
    }
    
    dec_yuv_frame = avcodec_alloc_frame();
    if (!dec_yuv_frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        DebugBreak();
        exit(1);
    }

    dec_rgb_frame = avcodec_alloc_frame();
    if (!dec_rgb_frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        DebugBreak();
        exit(1);
    }

    dec_rgb_frame->format = info.codec_fmt;
    dec_rgb_frame->width  = info.dst_width;
    dec_rgb_frame->height = info.dst_height;

    /* the image can be allocated by any means and av_image_alloc() is
     * just the most convenient way if av_malloc() is to be used */
    int ret = av_image_alloc(dec_rgb_frame->data, 
        dec_rgb_frame->linesize,
        dec_rgb_frame->width,
        dec_rgb_frame->height,
        (AVPixelFormat)dec_rgb_frame->format, 32);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate raw picture buffer\n");
        DebugBreak();
        exit(1);
    }

    int len, got_frame;

    len = avcodec_decode_video2(dec_ctx, dec_yuv_frame, &got_frame, &pkt);
    if (len < 0) {
        //fprintf(stderr, "Error while decoding frame %d\n", *frame_count);
        DebugBreak();
        exit(1);
    }
    if (got_frame) {
        //printf("Saving %sframe %3d\n", last ? "last " : "", *frame_count);
        fflush(stdout);
    }

    // Collect delayed frames
    pkt.data = NULL;
    pkt.size = 0;
    len = avcodec_decode_video2(dec_ctx, dec_yuv_frame, &got_frame, &pkt);
    if (len < 0) {
        //fprintf(stderr, "Error while decoding frame %d\n", *frame_count);
        DebugBreak();
        exit(1);
    }
    if (got_frame) {
        //printf("Saving %sframe %3d\n", last ? "last " : "", *frame_count);
        fflush(stdout);

        /* the picture is allocated by the decoder, no need to free it */
        //av_freep(&dec_yuv_frame->data[0]);

        ret = img_convert(
            (AVPicture*)dec_rgb_frame,
            info.codec_fmt,//AV_PIX_FMT_RGB24,
            (AVPicture*)dec_yuv_frame,
            PIX_FMT_YUV420P,
            dec_ctx->width,
            dec_ctx->height,
            info.dst_width,//CAPTURE_WIDTH,
            info.dst_height//CAPTURE_HEIGHT
            );
        if (ret < 0) {
            fprintf(stderr, "Could not convert raw rgb picture to yuv\n");
            DebugBreak();
            exit(1);
        }

        *info.dst_buf_size = dec_rgb_frame->linesize[0] * dec_rgb_frame->height;
        memcpy(pBufRGB24, 
            dec_rgb_frame->data[0],
            *info.dst_buf_size); 
    }

    av_free(inbuf);

    av_freep(&dec_rgb_frame->data[0]);
    avcodec_free_frame(&dec_rgb_frame);
    avcodec_free_frame(&dec_yuv_frame);

    avcodec_close(dec_ctx);
    av_free(dec_ctx);

    sm_cs.Unlock();
}