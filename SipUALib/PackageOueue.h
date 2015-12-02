#pragma once

#ifndef PACKAGE_QUEUE_H
#define PACKAGE_QUEUE_H

#include <memory>
#include <deque>
#include <stdint.h>//TODO:
#include <math.h>
#include <map>
#include <afxmt.h>
#include "AeccFreeList.h"


class BaseBuffer
{
#pragma push_macro("new")
#undef new
    AECC_DECLARE_FREELIST(BaseBuffer);
#pragma pop_macro("new")

public:
    BaseBuffer(BYTE* pBuf, int size);
    BaseBuffer(const BaseBuffer& other);
    virtual ~BaseBuffer();
    BYTE* getBuffer() { return m_pBuf; }
    int getSize() const { return m_nSize; }

protected:
    BYTE* m_pBuf;
    int m_nSize;
};

//typedef BaseBuffer AudioBuffer;

class AECInfo
{
public:
    AECInfo(double sysTimeRecordedOrPlayed)
        : m_sysTimeRecordedOrPlayed(sysTimeRecordedOrPlayed) {}
    virtual ~AECInfo() {}

    double GetRecordedOrPlayedTime() const { return m_sysTimeRecordedOrPlayed; }

protected:
    double m_sysTimeRecordedOrPlayed; // time by which audio buffer was recorded or played
};

class AudioBuffer : public BaseBuffer, public AECInfo
{
#pragma push_macro("new")
#undef new
    AECC_DECLARE_FREELIST(AudioBuffer);
#pragma pop_macro("new")

public:
    AudioBuffer(BYTE* pBuf, int size, double sysTimeRecordedOrPlayed);
    AudioBuffer(const AudioBuffer& other);
    virtual ~AudioBuffer();
};


class RTPPackageInfo
{
public:
    RTPPackageInfo(uint32_t seqNumber,
                   uint32_t extSeqNumber,
                   double sysTimeReceived)
                   : m_seqNumber(seqNumber),
                     m_extSeqNumber(extSeqNumber),
                     m_sysTimeReceived(sysTimeReceived) {}
    virtual ~RTPPackageInfo() {}

    uint32_t GetSequenceNumber() const { return m_seqNumber; }
    uint32_t GetExtendedSequenceNumber() const { return m_extSeqNumber; }
    double GetReceiveTime() const { return m_sysTimeReceived; }

protected:
    uint32_t m_seqNumber;
    uint32_t m_extSeqNumber;
    double m_sysTimeReceived;
};


class PlayBuffer : public BaseBuffer, public RTPPackageInfo
{
#pragma push_macro("new")
#undef new
    AECC_DECLARE_FREELIST(PlayBuffer);
#pragma pop_macro("new")

public:
    PlayBuffer(BYTE* pBuf, int size, uint32_t seqNumber, uint32_t extSeqNumber, double sysTimeReceived);
    PlayBuffer(const PlayBuffer& other);
    virtual ~PlayBuffer();
};


template<class ELEMENT_TYPE>
class PackageQueue
{
public:
    typedef ELEMENT_TYPE element_type;
    typedef const element_type const_element_type;
    typedef std::deque<element_type> queue_type;
    typedef typename queue_type::const_iterator const_iterator;
    typedef typename queue_type::iterator iterator;
    typedef typename queue_type::size_type size_type;
    typedef typename queue_type::const_reference const_reference;
    typedef typename queue_type::reference reference;

public:
    PackageQueue() {}
    virtual ~PackageQueue() {
        clear();
    }

    void clear()
    {
        m_cs.Lock();
        m_list.clear();
        m_cs.Unlock();
    }

    iterator begin()
    {
        m_cs.Lock();
        iterator it = m_list.begin();
        m_cs.Unlock();
        return it;
    }

    /*
    iterator end()
    {
        m_cs.Lock();
        iterator it = m_list.end();
        m_cs.Unlock();
        return it;
    }
    */

    const_reference at(size_type _Pos) const
    {
        m_cs.Lock();
        const_reference cRef = m_list.at(_Pos);
        m_cs.Unlock();
        return cRef;
    }
    reference at(size_type _Pos)
    {
        m_cs.Lock();
        reference ref = m_list.at(_Pos);
        m_cs.Unlock();
        return ref;
    }

    reference front()
    {
        m_cs.Lock();
        reference ref = m_list.front();
        m_cs.Unlock();
        return ref;
    }
    const_reference front() const
    {
        m_cs.Lock();
        const_reference ref = m_list.front();
        m_cs.Unlock();
        return ref;
    }

    size_type size() const
    {
        m_cs.Lock();
        size_type sz = m_list.size();
        m_cs.Unlock();
        return sz;
    }

    void push_back(const element_type pBuf)
    {
        m_cs.Lock();
        m_list.push_back(pBuf);
        m_cs.Unlock();
    }
    void pop_front()
    {
        m_cs.Lock();
        m_list.pop_front();
        m_cs.Unlock();
    }

private:
    mutable CCriticalSection m_cs;
    queue_type m_list;
};

typedef std::shared_ptr<AudioBuffer>    RecordPackage;
typedef std::shared_ptr<PlayBuffer>     PlayPackage;

typedef class PackageQueue<RecordPackage>   RecPkgQueue;
typedef class PackageQueue<PlayPackage>     PlayPkgQueue;


#ifdef ENABLE_AUDIO_AEC
class AECQueue
{
public:
    AECQueue(RecPkgQueue* pQueueMic, RecPkgQueue* pQueueRef)
        : m_pQueueMic(pQueueMic)
        , m_pQueueRef(pQueueRef)
    {
    }
    virtual ~AECQueue() {}

    void AddMicPackage(RecPkgQueue::element_type& mic)
    {
        m_cs.Lock();
        m_pQueueMic->push_back(mic);
        m_cs.Unlock();
    }
    void AddRefPackage(RecPkgQueue::element_type& ref)
    {
        m_cs.Lock();
        m_pQueueRef->push_back(ref);
        m_cs.Unlock();
    }

    bool FindMatch(double offset, double maxTimeInterval, int& idxMic, int& idxRef);

protected:
    RecPkgQueue* m_pQueueMic;
    RecPkgQueue* m_pQueueRef;

    mutable CCriticalSection m_cs;
};
#endif//ENABLE_AUDIO_AEC



//TODO: move to somewhere like videosetting/videocfg
typedef struct struVIDEOSAMPLEINFO{
    DWORD               m_biCompression;
    WORD                m_biBitCount;
    long                m_nWidth;
    long                m_nHeight;
    REFERENCE_TIME      m_AvgTimePerFrame;
    struVIDEOSAMPLEINFO(
        DWORD biCompression = 0,
        WORD biBitCount = 0,
        long nWidth = 0,
        long nHeight = 0,
        REFERENCE_TIME AvgTimePerFrame = 0
        ) : m_biCompression(biCompression),
            m_biBitCount(biBitCount),
            m_nWidth(nWidth),
            m_nHeight(nHeight),
            m_AvgTimePerFrame(AvgTimePerFrame)
    {
    }
} VIDEOSAMPLEINFO;

extern const VIDEOSAMPLEINFO* getVideoSampleInfo();
extern void setVideoSampleInfo(const VIDEOSAMPLEINFO&);


typedef struct struVIDEOFORMATINFO{
    int                 m_codecID; // AVCodecID
    long                m_nWidth;
    long                m_nHeight;
    REFERENCE_TIME      m_AvgTimePerFrame;
    struVIDEOFORMATINFO(
        int codecID = 0,
        long nWidth = 0,
        long nHeight = 0,
        REFERENCE_TIME AvgTimePerFrame = 0
        ) : m_codecID(codecID),
        m_nWidth(nWidth),
        m_nHeight(nHeight),
        m_AvgTimePerFrame(AvgTimePerFrame)
    {
    }
} VIDEOFORMATINFO;

extern const VIDEOFORMATINFO* getRemoteVideoFormatInfo();
extern void setRemoteVideoFormatInfo(const VIDEOFORMATINFO&);

typedef struct struVIDEOFRAMEHEADER {
    uint32_t m_nFrameIndex;
    struVIDEOFRAMEHEADER(uint32_t nFrameIndex)
        : m_nFrameIndex(nFrameIndex) {}
} VIDEOFRAMEHEADER;

class VideoFrame : public VIDEOFRAMEHEADER, public BaseBuffer, public RTPPackageInfo
{
#pragma push_macro("new")
#undef new
    AECC_DECLARE_FREELIST(VideoFrame);
#pragma pop_macro("new")

public:
    VideoFrame(
        uint32_t frameIndex
        ,BYTE * pBuffer
        ,long lBufferSize
        ,uint32_t seqNumber
        ,uint32_t extSeqNumber
        ,double sysTimeReceived
        ) : VIDEOFRAMEHEADER(frameIndex)
        , BaseBuffer(pBuffer, lBufferSize)
        , RTPPackageInfo(seqNumber, extSeqNumber, sysTimeReceived)
    {
    }
};


typedef struct struVIDEOPACKAGEHEADER : public VIDEOFRAMEHEADER {
    uint32_t m_nSubIndex;
    uint32_t m_nSubCount; // total # of sub packs of a single frame
    struVIDEOPACKAGEHEADER(
        uint32_t frameIndex
        ,uint32_t subIndex
        ,uint32_t nSubCount)
        : VIDEOFRAMEHEADER(frameIndex)
    {
        m_nSubIndex = subIndex;
        m_nSubCount = nSubCount;    }
} VIDEOPACKAGEHEADER;

class VideoPackage : public VIDEOPACKAGEHEADER, public BaseBuffer, public RTPPackageInfo
{
#pragma push_macro("new")
#undef new
    AECC_DECLARE_FREELIST(VideoPackage);
#pragma pop_macro("new")

public:
    VideoPackage(
        uint32_t frameIndex
        ,uint32_t subIndex
        ,uint32_t subCount
        ,BYTE * pBuffer
        ,long lBufferSize
        ,uint32_t seqNumber
        ,uint32_t extSeqNumber
        ,double sysTimeReceived
        ) : VIDEOPACKAGEHEADER(frameIndex, subIndex, subCount)
        , BaseBuffer(pBuffer, lBufferSize)
        , RTPPackageInfo(seqNumber, extSeqNumber, sysTimeReceived)
    {
    }
};

typedef std::shared_ptr<VideoFrame>     VideoFrmType;
typedef class PackageQueue<VideoFrmType>   VideoFrmQueue;

typedef std::shared_ptr<VideoPackage>     VideoPkgType;
typedef class PackageQueue<VideoPkgType>   VideoPkgQueue;

//typedef std::shared_ptr<VideoPkgQueue> VideoPkgofSingleFrame;
typedef std::map<uint32_t, VideoPkgType> VideoPkgofSingleFrame;
typedef std::map<uint32_t, std::shared_ptr<VideoPkgofSingleFrame>> VideoFramePackages;


#endif//PACKAGE_QUEUE_H