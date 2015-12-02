#ifndef PUSH_SOURCE_H
#define PUSH_SOURCE_H

#include <strsafe.h>
#include "PackageOueue.h"
//#include "VideoCapture.h"
#include <mtype.h>
#include <reftime.h>
#include <wxdebug.h>
#include <combase.h>
#include <wxlist.h>
#include <wxutil.h>
#include <amfilter.h>
#include <source.h>

#include "Worker.h"

const REFERENCE_TIME FPS_RATE = UNITS / FRAMES_PER_SECOND;

// Filter name strings
#define g_wszPushBitmap     L"PushSource Bitmap Filter"




class CPushPinBitmap : public CSourceStream, public CWorker
{
    typedef CSourceStream super;
protected:

    int m_FramesWritten;				// To track where we are in the file
    BOOL m_bZeroMemory;                 // Do we need to clear the buffer?
    CRefTime m_rtSampleTime;	        // The time stamp for each sample

    int m_iFrameNumber;
    const REFERENCE_TIME m_rtFrameLength;

    //RECT m_rScreen;                     // Rect containing entire screen coordinates

    //int m_iImageHeight;                 // The current image height
    //int m_iImageWidth;                  // And current image width
    //int m_iRepeatTime;                  // Time in msec between frames
    //int m_nCurrentBitDepth;             // Screen bit depth

    //CMediaType m_MediaType;
    CCritSec m_cSharedState;            // Protects our internal state
    //CImageDisplay m_Display;            // Figures out our media type for us

public:

    CPushPinBitmap(HRESULT *phr, CSource *pFilter);
    ~CPushPinBitmap();

    void SetQueue(VideoFrmQueue* pQueue);
    virtual void StopPlay() {
        CWorker::Stop();
    }

    void onPossibleDelay();

    // Override the version that offers exactly one media type
    HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pRequest);
    HRESULT FillBuffer(IMediaSample *pSample);
    
    // Set the agreed media type and set up the necessary parameters
    HRESULT SetMediaType(const CMediaType *pMediaType);

    // Support multiple display formats
    HRESULT CheckMediaType(const CMediaType *pMediaType);
    HRESULT GetMediaType(int iPosition, CMediaType *pmt);

    // Quality control
	// Not implemented because we aren't going in real time.
	// If the file-writing filter slows the graph down, we just do nothing, which means
	// wait until we're unblocked. No frames are ever dropped.
    STDMETHODIMP Notify(IBaseFilter *pSelf, Quality q)
    {
        return E_FAIL;
    }

    VideoFrmQueue* m_pQueue;
    //bool m_bWorking;
};



class CPushSourceBitmap : public CSource
{
    typedef CSource super;

private:
    // Constructor is private because you have to use CreateInstance
    CPushSourceBitmap(IUnknown *pUnk, HRESULT *phr);
    ~CPushSourceBitmap();

    CPushPinBitmap *m_pPin;
    VideoFrmQueue* m_pQueue;

public:
    static CUnknown * WINAPI CreateInstance(IUnknown *pUnk, HRESULT *phr);  

    void SetQueue(VideoFrmQueue* pQueue);

    void StopPlay() {
        m_pPin->StopPlay();
    }

    //STDMETHODIMP Stop() {
    //    //m_pPin->StopPlay();
    //    return super::Stop();
    //}
    //STDMETHODIMP Pause();
};

#endif//PUSH_SOURCE_H