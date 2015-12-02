#include "stdafx.h"
#include <streams.h>

#include "PushSource.h"
#include "PushGuids.h"
//#include "DibHelper.h"
#include "Config.h"
#include "dvdmedia.h"
#include <jrtplib3/rtptimeutilities.h>



CPushPinBitmap::CPushPinBitmap(HRESULT *phr, CSource *pFilter)
        : CSourceStream(NAME("Push Source Bitmap"), phr, pFilter, L"Out"),
        m_FramesWritten(0),
        m_bZeroMemory(0),
        m_iFrameNumber(0),
        m_rtFrameLength(FPS_RATE)
{
}

CPushPinBitmap::~CPushPinBitmap()
{   
    DbgLog((LOG_TRACE, 3, TEXT("Frames written %d"), m_iFrameNumber));
}

void CPushPinBitmap::SetQueue(VideoFrmQueue* pQueue)
{
    m_pQueue = pQueue;
}

HRESULT CPushPinBitmap::GetMediaType(int iPosition, CMediaType *pmt)
{
    if (!IsWorking())
        return E_FAIL;

    CheckPointer(pmt,E_POINTER);
    CAutoLock cAutoLock(m_pFilter->pStateLock());

    if(iPosition < 0)
        return E_INVALIDARG;

    // Have we run off the end of types?
    if(iPosition > 0)
        return VFW_S_NO_MORE_ITEMS;

    VIDEOINFO *pvi = (VIDEOINFO *) pmt->AllocFormatBuffer(sizeof(VIDEOINFO));
    if(NULL == pvi)
        return(E_OUTOFMEMORY);

    // Initialize the VideoInfo structure before configuring its members
    ZeroMemory(pvi, sizeof(VIDEOINFO));

    // Return our 24bit format
    pvi->bmiHeader.biCompression = BI_RGB;
    pvi->bmiHeader.biBitCount    = CAPTURE_BIBITCOUNT;

    // wait for buffer to come in
    while ( IsWorking() && m_pQueue->size() == 0 )
        ::Sleep(1);

    if ( !IsWorking() )
        return E_FAIL;

    char szInfo[256];
#ifdef PRINT_AV_INFO
    sprintf_s(szInfo, "\nGot frame %d in CPushPinDesktop::GetMediaType",
        m_pQueue->size());
    DEBUG_INFO(szInfo);
#endif//PRINT_AV_INFO

    //TODO: no need cRef
    VideoFrmQueue::const_reference cRef = m_pQueue->front();

    //TODO: get width/h from mCB
    long w=CAPTURE_WIDTH, h = CAPTURE_HEIGHT;
    // Adjust the parameters common to all formats
    pvi->bmiHeader.biSize       = sizeof(BITMAPINFOHEADER);
    pvi->bmiHeader.biWidth      = w;
    pvi->bmiHeader.biHeight     = h;
    pvi->bmiHeader.biPlanes     = 1;
    pvi->bmiHeader.biSizeImage  = cRef->getSize();
    pvi->bmiHeader.biClrImportant = 0;

    SetRectEmpty(&(pvi->rcSource)); // we want the whole image area rendered.
    SetRectEmpty(&(pvi->rcTarget)); // no particular destination rectangle

    pmt->SetType(&MEDIATYPE_Video);
    pmt->SetFormatType(&FORMAT_VideoInfo);
    pmt->SetTemporalCompression(FALSE);

    // Work out the GUID for the subtype from the header info.
    const GUID SubTypeGUID = GetBitmapSubtype(&pvi->bmiHeader);
    pmt->SetSubtype(&SubTypeGUID);
    pmt->SetSampleSize(pvi->bmiHeader.biSizeImage);

    return NOERROR;
}

HRESULT CPushPinBitmap::CheckMediaType(const CMediaType *pMediaType)
{
    if (!IsWorking())
        return E_FAIL;

    CheckPointer(pMediaType,E_POINTER);

    if((*(pMediaType->Type()) != MEDIATYPE_Video) ||   // we only output video
        !(pMediaType->IsFixedSize()))                  // in fixed size samples
    {                                                  
        return E_INVALIDARG;
    }

    // Check for the subtypes we support
    const GUID *SubType = pMediaType->Subtype();
    if (SubType == NULL)
        return E_INVALIDARG;

    if((*SubType != MEDIASUBTYPE_RGB24))
    {
        return E_INVALIDARG;
    }

    return S_OK;  // This format is acceptable.
}

HRESULT CPushPinBitmap::DecideBufferSize(IMemAllocator *pAlloc,
                                      ALLOCATOR_PROPERTIES *pProperties)
{
    //if (!IsWorking())
    //    return E_FAIL;

    CheckPointer(pAlloc,E_POINTER);
    CheckPointer(pProperties,E_POINTER);

    CAutoLock cAutoLock(m_pFilter->pStateLock());
    HRESULT hr = NOERROR;

    VIDEOINFO *pvi = (VIDEOINFO *) m_mt.Format();
    pProperties->cBuffers = 1;
    pProperties->cbBuffer = pvi->bmiHeader.biSizeImage;

    ASSERT(pProperties->cbBuffer);

    // Ask the allocator to reserve us some sample memory. NOTE: the function
    // can succeed (return NOERROR) but still not have allocated the
    // memory that we requested, so we must check we got whatever we wanted.
    ALLOCATOR_PROPERTIES Actual;
    hr = pAlloc->SetProperties(pProperties,&Actual);
    if(FAILED(hr))
    {
        return hr;
    }

    // Is this allocator unsuitable?
    if(Actual.cbBuffer < pProperties->cbBuffer)
    {
        return E_FAIL;
    }

    // Make sure that we have only 1 buffer (we erase the ball in the
    // old buffer to save having to zero a 200k+ buffer every time
    // we draw a frame)
    ASSERT(Actual.cBuffers == 1);

    return NOERROR;
}

HRESULT CPushPinBitmap::SetMediaType(const CMediaType *pMediaType)
{
    //if (!IsWorking())
    //    return E_FAIL;

    CAutoLock cAutoLock(m_pFilter->pStateLock());

    // Pass the call up to my base class
    HRESULT hr = CSourceStream::SetMediaType(pMediaType);

    if(SUCCEEDED(hr))
    {
        VIDEOINFO * pvi = (VIDEOINFO *) m_mt.Format();
        if (pvi == NULL)
            return E_UNEXPECTED;

        switch(pvi->bmiHeader.biBitCount)
        {
        case 24:    // RGB24
            // Save the current media type and bit depth
            //m_MediaType = *pMediaType;
            //m_nCurrentBitDepth = pvi->bmiHeader.biBitCount;
            hr = S_OK;
            break;

        default:
            // We should never agree any other media types
            ASSERT(FALSE);
            hr = E_INVALIDARG;
            break;
        }
    }

    return hr;

}

void CPushPinBitmap::onPossibleDelay()
{
    char szInfo[256];
    double dCurTime = jrtplib::RTPTime::CurrentTime().GetDouble();
    while (IsWorking()
        && m_pQueue->size() > VIDEO_PLAY_FRAME_BUFFER_SIZE_LIMIT)
    {
        VideoFrmQueue::reference pBuf = m_pQueue->front();
        if (dCurTime - pBuf->GetReceiveTime() > VIDEO_PLAY_DELAY_PERMITTED)
        {
#ifdef PRINT_AV_INFO
            sprintf_s(szInfo,
                "Video frame(#%d) abandoned.",
                pBuf->m_nFrameIndex);
            DEBUG_INFO(szInfo);
#endif//PRINT_AV_INFO
            m_pQueue->pop_front();
        }
        else
            break;
    }
}

HRESULT CPushPinBitmap::FillBuffer(IMediaSample *pSample)
{
    if (!IsWorking())
    {
        return S_FALSE;// S_FALSE indicating end of stream
    }

    BYTE *pData;
    long cbData;

    CheckPointer(pSample, E_POINTER);

    CAutoLock cAutoLockShared(&m_cSharedState);

    // Access the sample's data buffer
    pSample->GetPointer(&pData);
    cbData = pSample->GetSize();

    // Check that we're still using video
    ASSERT(m_mt.formattype == FORMAT_VideoInfo);

    VIDEOINFOHEADER *pVih = (VIDEOINFOHEADER*)m_mt.pbFormat;

    // Copy the DIB bits over into our filter's output buffer.
    // Since sample size may be larger than the image size, bound the copy size.
    int nSize = min(pVih->bmiHeader.biSizeImage, (DWORD) cbData);
    //HDIB hDib = CopyScreenToBitmap(&m_rScreen, pData, (BITMAPINFO *) &(pVih->bmiHeader));

    //if (hDib)
    //    DeleteObject(hDib);

    // wait for buffer to come in
    while ( IsWorking() && m_pQueue->size() == 0 )
        ::Sleep(1);

    if (!IsWorking())
    {
        return S_FALSE;
    }

#ifdef REMOVE_VIDEO_DELAY
    if ( 0 == (m_iFrameNumber % FRAMES_PER_SECOND) )
        this->onPossibleDelay();
#endif//REMOVE_VIDEO_DELAY

    VideoFrmQueue::const_reference cRef = m_pQueue->front();
    const REFERENCE_TIME frameLength  = getRemoteVideoFormatInfo()->m_AvgTimePerFrame;
    memcpy(pData, cRef->getBuffer(), nSize);
    m_pQueue->pop_front();


    // Set the timestamps that will govern playback frame rate.
    // If this file is getting written out as an AVI,
    // then you'll also need to configure the AVI Mux filter to 
    // set the Average Time Per Frame for the AVI Header.
    // The current time is the sample's start.
    //REFERENCE_TIME rtStart = m_iFrameNumber * m_rtFrameLength;
    //REFERENCE_TIME rtStop  = rtStart + m_rtFrameLength;

    REFERENCE_TIME rtStart = m_iFrameNumber * frameLength;
    REFERENCE_TIME rtStop  = rtStart + frameLength;

    pSample->SetTime(&rtStart, &rtStop);
    m_iFrameNumber++;

    // Set TRUE on every sample for uncompressed frames
    pSample->SetSyncPoint(TRUE);

    return S_OK;
}


CPushSourceBitmap::CPushSourceBitmap(IUnknown *pUnk, HRESULT *phr)
           : CSource(NAME("PushSourceBitmap"), pUnk, CLSID_PushSourceBitmap)
{
    // The pin magically adds itself to our pin array.
    m_pPin = new CPushPinBitmap(phr, this);

	if (phr)
	{
		if (m_pPin == NULL)
			*phr = E_OUTOFMEMORY;
		else
        {
            m_pPin->Start();
			*phr = S_OK;
        }
	}  
}

CPushSourceBitmap::~CPushSourceBitmap()
{
    delete m_pPin;
}

CUnknown * WINAPI CPushSourceBitmap::CreateInstance(IUnknown *pUnk, HRESULT *phr)
{
    CPushSourceBitmap *pNewFilter = new CPushSourceBitmap(pUnk, phr );

	if (phr)
	{
		if (pNewFilter == NULL) 
			*phr = E_OUTOFMEMORY;
		else
			*phr = S_OK;
	}
    return pNewFilter;
}

void CPushSourceBitmap::SetQueue(VideoFrmQueue* pQueue)
{
    m_pQueue = pQueue;
    m_pPin->SetQueue(pQueue);
}