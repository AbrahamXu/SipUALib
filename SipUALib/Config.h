#pragma once

#ifndef CONFIG_H
#define CONFIG_H


//<<<<<<<<<<<<<<<<<<<<< test settings

//#define ENABLE_AUDIO_DENOISE //TODO: no use, remove it
#define ENABLE_AUDIO_AEC // if disabled, recorded data will be pushed into out queue directly;
                         // otherwise, recorded data will be cached in mic queue and
                         //   a. if UseAEC is false, pull from mic queue and then push into out queue for out-sending;
                         //   b. if UseAEC is true, then sync record/play data and do AEC. After that, push result into out queue for sending out.
//#define PRINT_AEC_INFO // AEC related log

#define SIPUA_REG_EXPIRE 3600

#define RTP_SUPPORT_MEMORYMANAGEMENT // enable rtp mem mgr

#define FORMAT_INFO
//#define ENABLE_PRINT_LOG
#define LOG_DBG_INFO // for dbg string log to file
//#ifdef _DEBUG
//  #define ENABLE_PRINT_LOG
//#endif//_DEBUG

#ifdef ENABLE_PRINT_LOG
  #define FORMAT_INFO
  #ifdef FORMAT_INFO
    #define PRINT_DBG_INFO // for dbg string output
    #define LOG_DBG_INFO // for dbg string log to file
  #endif//FORMAT_INFO
#endif//ENABLE_PRINT_LOG

//#define ENABLE_QUEUE_CLEAR // clear buffer queue before closing

//#define PRINT_AV_INFO // to print out a/v packet info

// for testing
#define RTP_AUDIO_SENDRECV // enable audio send/recv testing
#define RTP_VIDEO_SENDER // enable video send testing
#define RTP_VIDEO_RECEIVER // enable video recv testing

#define VIDEO_BIT_RATE 200000 // bit rate, for encode

#define REMOVE_AUDIO_DELAY // enable to remote audio latency
#ifdef REMOVE_AUDIO_DELAY
#define AUDIO_PLAY_DELAY_PERMITTED 0.1 // seconds
#endif//REMOVE_AUDIO_DELAY

#define REMOVE_VIDEO_DELAY // enable to remote video latency
#ifdef REMOVE_VIDEO_DELAY
#define VIDEO_PLAY_DELAY_PERMITTED 0.2 // seconds
#define VIDEO_PLAY_FRAME_BUFFER_SIZE_LIMIT 2
#endif//REMOVE_VIDEO_DELAY

//>>>>>>>>>>>>>>>>>>>>> test settings


//<<<<<<<<<<<<<<<<<<<<< audio settings

#define BITS_PER_BYTE                           8
#define SAMPLES_PER_SECOND                      8000
#define BITS_PER_SAMPLE                         16
#define BYTES_PER_SAMPLE                        BITS_PER_SAMPLE / BITS_PER_BYTE
#define AVERAGE_BYTES_PER_SECOND                SAMPLES_PER_SECOND * BITS_PER_SAMPLE / BITS_PER_BYTE
#define RECORD_SEND_BUFFER_SIZE                 256
#define RECORD_BUFFER_SIZE                      256
#define SAMPLES_PER_RECORD                      RECORD_BUFFER_SIZE / BYTES_PER_SAMPLE
#define TIME_PER_SECOND                         1000
#define RECORD_SEND_BUFFER_SENTOUT_PER_SECOND   (AVERAGE_BYTES_PER_SECOND / RECORD_SEND_BUFFER_SIZE)
#define RECORD_PACKAGE_DIVISION_COUNT           (RECORD_BUFFER_SIZE / RECORD_SEND_BUFFER_SIZE)
#define TIMESTAMP_INC_UNIT                      (TIME_PER_SECOND / RECORD_SEND_BUFFER_SENTOUT_PER_SECOND)

#define PAYLOAD_TYPE_AUDIO  0
#define MARK_FLAG_AUDIO     false
#define TIMESTAMP_TO_INC    TIMESTAMP_INC_UNIT

#define SOUND_RECORD_BUFFER_COUNT  8
#define SOUND_PLAY_BUFFER_COUNT    8

//>>>>>>>>>>>>>>>>>>>>> audio settings



//<<<<<<<<<<<<<<<<<<<<< video settings

#define VIDEO_SEND_BUFFER_SIZE 200
//#define VIDEO_FRAME_SIZE (CAPTURE_WIDTH * CAPTURE_HEIGHT * 3)
//#define VIDEO_PACKS_PER_FRAME  VIDEO_FRAME_SIZE / VIDEO_SEND_BUFFER_SIZE


#define SKIP_FRAMES
#define FRAMES_PER_SECOND   15

#define PAYLOAD_TYPE_VIDEO 1
#define MARK_FLAG_VIDEO 0
//#define TIMESTAMP_TO_INC 100
//#define TIMESTAMP_INC_UNIT TIMESTAMP_TO_INC


#define CAPTURE_WIDTH   640
#define CAPTURE_HEIGHT  480
#define CAPTURE_SUBTYPE MEDIASUBTYPE_RGB24;//MEDIASUBTYPE_RGB555
#define CAPTURE_PIX_FORMAT AV_PIX_FMT_RGB24
#define CAPTURE_BICOMPRESSION   BI_RGB
#define CAPTURE_BIBITCOUNT      24//16

#define H264_WIDTH 352
#define H264_HEIGHT 288
#define CODEC_FORMAT AV_CODEC_ID_H264//remove it

//>>>>>>>>>>>>>>>>>>>>> video settings




//======================== Copy from speex
// Microsoft version of 'inline'
#define inline __inline

// Visual Studio support alloca(), but it always align variables to 16-bit
// boundary, while SSE need 128-bit alignment. So we disable alloca() when
// SSE is enabled.
#ifndef _USE_SSE
#  define USE_ALLOCA
#endif

/* Default to floating point */
#ifndef FIXED_POINT
#  define FLOATING_POINT
#  define USE_SMALLFT
#else
#  define USE_KISS_FFT
#endif

/* We don't support visibility on Win32 */
#define EXPORT
//======================== Copy from speex

#endif//CONFIG_H