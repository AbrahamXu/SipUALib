#pragma once

#ifndef VIDEO_CAPTURE_H
#define VIDEO_CAPTURE_H

#include "PackageOueue.h"



class CSampleGrabberCB : public ISampleGrabberCB 
{
public:
    VideoFrmQueue*        m_pFrames;
public:
    CSampleGrabberCB(){
        m_pFrames            = NULL;
    }

    void SetQueue(VideoFrmQueue* queue) { m_pFrames = queue; }

    STDMETHODIMP_(ULONG) AddRef() { return 2; }
    STDMETHODIMP_(ULONG) Release() { return 1; }
    STDMETHODIMP QueryInterface(REFIID riid, void ** ppv) {
        if( riid == IID_ISampleGrabberCB || riid == IID_IUnknown ){ 
            *ppv = (void *) static_cast<ISampleGrabberCB*> ( this );
            return NOERROR;
        } 
        return E_NOINTERFACE;
    }

    STDMETHODIMP SampleCB( double SampleTime, IMediaSample * pSample )  {
        return 0;
    }

    STDMETHODIMP BufferCB(double SampleTime,
                          BYTE *pBuffer,
                          long BufferLen)
    {
        if (!pBuffer) return E_POINTER;
        {
            static int siFrame = 0;
#ifdef SKIP_FRAMES
            //TODO: 30 fps is too high, reduce to 10 fps
            int iTemp = siFrame % 2;
            if (iTemp != 0)
            {
                siFrame++;
                return 0;
            }
#endif//SKIP_FRAMES
            VideoFrmType frame(new VideoFrame(
                siFrame,
                pBuffer,
                BufferLen
                ,0,0,0)); //TODO: remote un-necessary params
            m_pFrames->push_back(frame);

            siFrame++;
        }
        return 0;
    }
};



class VideoCapture 
{
public:
    friend class CSampleGrabberCB;
    VideoCapture();
    virtual ~VideoCapture();

    int EnumDevices(bool bVideo=true, bool bInput=true);
    HRESULT GetPreviewInfo(int iDeviceID,
                           VIDEOSAMPLEINFO& vsi);
    HRESULT StartPreview(int iDeviceID, HWND hWnd, VideoFrmQueue* pQueue);
    HRESULT StopPreview();

    HRESULT GetCaptureRatio(IBaseFilter* pCapFilter, ICaptureGraphBuilder2* pBuild);

    void ConfigCameraPin(HWND hwndParent);
    void ConfigCameraFilter(HWND hwndParent);

private:
    HWND m_hWnd;
    CComPtr<IGraphBuilder>          m_pGB;
    CComPtr<ICaptureGraphBuilder2> m_pCapture;
    CComPtr<IBaseFilter>      m_pBF;
    CComPtr<IMediaControl>   m_pMC;
    CComPtr<IVideoWindow>   m_pVW;
    CComPtr<ISampleGrabber>m_pGrabber;

protected:
    bool BindFilter(int deviceId, IBaseFilter **pFilter);
    void ResizeVideoWindow();
    HRESULT SetupVideoWindow();
    HRESULT InitCaptureGraphBuilder();

    CSampleGrabberCB m_sampleCB;
};

#endif//VIDEO_CAPTURE_H