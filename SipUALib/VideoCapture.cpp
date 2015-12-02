#include "StdAfx.h"
#include "VideoCapture.h"
#include "Config.h"
#include <mtype.h>
#include <wxdebug.h>
#include <wxutil.h>
#include <dvdmedia.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

VideoCapture::VideoCapture()
{
    CoInitialize(NULL);
    m_hWnd = NULL;
    m_pVW = NULL;
    m_pMC = NULL;
    m_pGB = NULL;
    m_pBF = NULL;
    m_pCapture = NULL;
    m_pGrabber=NULL;
}

VideoCapture::~VideoCapture()
{
    if(m_pMC)
        m_pMC->Stop();
    if(m_pVW)
    {
        m_pVW->put_Visible(OAFALSE);
        m_pVW->put_Owner(NULL);
    }
    m_pVW = NULL;
    m_pMC = NULL;
    m_pGB = NULL;
    m_pBF = NULL;
    m_pCapture = NULL;
    m_pGrabber=NULL;

    CoUninitialize();
}

int VideoCapture::EnumDevices(bool bVideo/*=true*/, bool bInput/*=true*/)
{
    int id = 0;

    ICreateDevEnum *pCreateDevEnum;
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, 
        CLSCTX_INPROC_SERVER,
        IID_ICreateDevEnum,
        (void**)&pCreateDevEnum);

    if (hr != NOERROR)  return -1;

    IEnumMoniker *pEm;
    // Video type enumerate
    if (bVideo)
    {
        if (bInput)
            hr = pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory,&pEm, 0);
        else
            ASSERT(false);
    }
    // Audio type enumerate
    else
    {
        // Waveout audio renderer
        // CLSID_AudioRender

        // DSound audio renderer
        // CLSID_DSoundRender

        // Wavein audio recorder
        // CLSID_AudioRecord

        if (bInput)
            hr = pCreateDevEnum->CreateClassEnumerator(CLSID_AudioInputDeviceCategory,&pEm, 0);
        else
            hr = pCreateDevEnum->CreateClassEnumerator(CLSID_AudioRendererCategory,&pEm, 0);
    }

    if (hr != NOERROR) return -1;
    // enumerator reset
    pEm->Reset();
    ULONG cFetched;
    IMoniker *pM;
    while(hr = pEm->Next(1, &pM, &cFetched), hr==S_OK)
    {
        IPropertyBag *pBag;
        // Device property bag
        hr = pM->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pBag);
        if(SUCCEEDED(hr)) 
        {
            VARIANT var;
            var.vt = VT_BSTR;
            hr = pBag->Read(L"FriendlyName", &var, NULL);
            if(hr == NOERROR) 
            {
#ifdef PRINT_AV_INFO
                SPRINTF_S(dbg_str,
                    "======Enumerating supported device [%d]:"
                    "\n%s"
                    , id
                    , WString2String(var.bstrVal).c_str()
                    );
                DEBUG_INFO(dbg_str);
#endif//PRINT_AV_INFO
                SysFreeString(var.bstrVal);
                id++;
            }
            pBag->Release();
        }
        pM->Release();
    }
    return id;
}

HRESULT VideoCapture::GetCaptureRatio(IBaseFilter* pCapFilter, ICaptureGraphBuilder2* pBuild)
{
    static char szInfo[512] = {0};
#ifdef PRINT_AV_INFO
    SPRINTF_S(szInfo,
        "\n======Enumerating supported video formats:\n"
        );
    DEBUG_INFO(szInfo);
#endif//PRINT_AV_INFO

    HRESULT hr;
    CComPtr<IAMStreamConfig> pam;

    hr = pBuild->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video,
        pCapFilter, IID_IAMStreamConfig, reinterpret_cast<void**>(&pam));

    int nCount = 0;
    int nSize = 0;
    hr = pam->GetNumberOfCapabilities(&nCount, &nSize);

    if (sizeof(VIDEO_STREAM_CONFIG_CAPS) == nSize) {
        for (int i=0; i<nCount; i++) {
            VIDEO_STREAM_CONFIG_CAPS scc;
            AM_MEDIA_TYPE* pmmt;

            hr = pam->GetStreamCaps(i, &pmmt, reinterpret_cast<BYTE*>(&scc));

            if (pmmt->formattype == FORMAT_VideoInfo)
            {
                VIDEOINFOHEADER* pvih = reinterpret_cast<VIDEOINFOHEADER*>(pmmt->pbFormat);
                //int nFrame = pvih->AvgTimePerFrame;
                //ASSERT(nFrame > 0);
#ifdef PRINT_AV_INFO
                SPRINTF_S(szInfo,
                    "\n------video format[%d]:"
                    "\n    frm_rate: %d"
                    "\n    bit_rate: %d kbs"
                    "\n    width: %d"
                    "\n    height: %d"
                    "\n    bit-count: %d"
                    "\n    compression: %d"
                    "\n    img size: %d kByte"
                    ,i
                    ,(int)(10000000 / pvih->AvgTimePerFrame)
                    ,pvih->dwBitRate/1024
                    ,pvih->bmiHeader.biWidth
                    ,pvih->bmiHeader.biHeight
                    ,pvih->bmiHeader.biBitCount
                    ,pvih->bmiHeader.biCompression
                    ,pvih->bmiHeader.biSizeImage/1024
                    );
                DEBUG_INFO(szInfo);
#endif//PRINT_AV_INFO
            }
            else if (pmmt->formattype == FORMAT_VideoInfo2)
            {
                VIDEOINFOHEADER2* pvih = reinterpret_cast<VIDEOINFOHEADER2*>(pmmt->pbFormat);
                //int nFrame = pvih->AvgTimePerFrame;
                //ASSERT(nFrame > 0);
#ifdef PRINT_AV_INFO
                SPRINTF_S(szInfo,
                    "\n------video format[%d]:"
                    "\n    frm_rate: %d"
                    "\n    bit_rate: %d kbs"
                    "\n    width: %d"
                    "\n    height: %d"
                    "\n    bit-count: %d"
                    "\n    compression: %d"
                    "\n    img size: %d kByte"
                    ,i
                    ,(int)(10000000 / pvih->AvgTimePerFrame)
                    ,pvih->dwBitRate/1024
                    ,pvih->bmiHeader.biWidth
                    ,pvih->bmiHeader.biHeight
                    ,pvih->bmiHeader.biBitCount
                    ,pvih->bmiHeader.biCompression
                    ,pvih->bmiHeader.biSizeImage/1024
                    );
                DEBUG_INFO(szInfo);
#endif//PRINT_AV_INFO
            }
        }
    }

    return hr;
}
/*
HRESULT VideoCapture::SetCaptureRatio(IBaseFilter* pCapFilter, ICaptureGraphBuilder2* pBuild, int nWidth, int nHeight, int nFrmRate)
{
    static char szInfo[512] = {0};
    SPRINTF_S(szInfo,
        "\n======Setting video formats:\n"
        );
    DEBUG_INFO(szInfo);

    HRESULT hr;
    CComPtr<IAMStreamConfig> pVSC;

    hr = pBuild->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video,
        pCapFilter, IID_IAMStreamConfig, reinterpret_cast<void**>(&pVSC));

    int nCount = 0;
    int nSize = 0;
    AM_MEDIA_TYPE *pmt;

    // default capture format
    if(pVSC && pVSC->GetFormat(&pmt) == S_OK)
    {
        // DV capture does not use a VIDEOINFOHEADER
        if(pmt->formattype == FORMAT_VideoInfo)
        {
            // resize our window to the default capture size
            //ResizeWindow(HEADER(pmt->pbFormat)->biWidth,
            //    ABS(HEADER(pmt->pbFormat)->biHeight));
            VIDEOINFOHEADER* pvih = reinterpret_cast<VIDEOINFOHEADER*>(pmt->pbFormat);
            SPRINTF_S(szInfo,
                "\n------using video format:"
                "\n    frm_rate: %d"
                "\n    bit_rate: %d kbs"
                "\n    width: %d"
                "\n    height: %d"
                "\n    bit-count: %d"
                "\n    compression: %d"
                "\n    img size: %d kByte"
                ,(int)(10000000 / pvih->AvgTimePerFrame)
                ,pvih->dwBitRate/1024
                ,pvih->bmiHeader.biWidth
                ,pvih->bmiHeader.biHeight
                ,pvih->bmiHeader.biBitCount
                ,pvih->bmiHeader.biCompression
                ,pvih->bmiHeader.biSizeImage/1024
                );
            DEBUG_INFO(szInfo);



            pvih->dwBitRate = 160 * 120 * 2 * 30;
            pvih->bmiHeader.biWidth = 160;
            pvih->bmiHeader.biHeight = 120;
            pvih->bmiHeader.biBitCount = 16;
            pvih->bmiHeader.biCompression = 844715353;
            pvih->bmiHeader.biSizeImage = 160 * 120 * 2;

            hr = pVSC->SetFormat(pmt);

        }
        if(pmt->majortype != MEDIATYPE_Video)
        {
            // This capture filter captures something other that pure video.
            // Maybe it's DV or something?  Anyway, chances are we shouldn't
            // allow capturing audio separately, since our video capture
            // filter may have audio combined in it already!
        }
        DeleteMediaType(pmt);
    }

    return hr;
}
*/

HRESULT VideoCapture::GetPreviewInfo(int iDeviceID,
                                     VIDEOSAMPLEINFO& vsi)
{
    HRESULT hr;
    hr = InitCaptureGraphBuilder();
    if (FAILED(hr)){
        DEBUG_INFO("Failed to get video interfaces!");
        return hr;
    }
    // Bind Device Filter. We know the device because the id was passed in
    if(!BindFilter(iDeviceID, &m_pBF))return S_FALSE;
    hr = m_pGB->AddFilter(m_pBF, L"Capture Filter");

    //TODO: test for frm rate, etc
    hr = GetCaptureRatio(m_pBF, m_pCapture);
    //hr = SetCaptureRatio(m_pBF, m_pCapture, CAPTURE_WIDTH, CAPTURE_HEIGHT, 10);
    //hr = GetCaptureRatio(m_pBF, m_pCapture);


    //hr = m_pCapture->RenderStream(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video, 
    // m_pBF, NULL, NULL);
    // create a sample grabber
    //hr = CoCreateInstance( CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER, IID_ISampleGrabber, (void**)&m_pGrabber );
    m_pGrabber.CoCreateInstance(CLSID_SampleGrabber);
    if(FAILED(hr)){
        DEBUG_INFO("Fail to create SampleGrabber, maybe qedit.dll is not registered?");
        return hr;
    }
    CComQIPtr< IBaseFilter, &IID_IBaseFilter > pGrabBase( m_pGrabber );
    // set video format
    //AM_MEDIA_TYPE mt;
    CMediaType mt;
    ZeroMemory(&mt, sizeof(AM_MEDIA_TYPE));
    mt.majortype = MEDIATYPE_Video;
    mt.subtype = CAPTURE_SUBTYPE;
    /*
    //// Work out the GUID for the subtype from the header info.
    //const GUID SubTypeGUID = GetBitmapSubtype(&pvi->bmiHeader);
    //mt.SetSubtype(&SubTypeGUID);
    //mt.SetSampleSize(pvi->bmiHeader.biSizeImage);
    */

    hr = m_pGrabber->SetMediaType(&mt);

    if( FAILED(hr) ){
        DEBUG_INFO("Fail to set media type!");
        return hr;
    }
    //TODO: test for frm rate, etc
    //hr = GetCaptureRatio(m_pBF, m_pCapture);
    //hr = SetCaptureRatio(m_pBF, m_pCapture, CAPTURE_WIDTH, CAPTURE_HEIGHT, 10);


    hr = m_pGB->AddFilter( pGrabBase, L"Grabber" );
    if( FAILED( hr ) ){
        DEBUG_INFO("Fail to put sample grabber in graph");
        return hr;
    }

    // try to render preview/capture pin
    hr = m_pCapture->RenderStream(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video,m_pBF,pGrabBase,NULL);
    if( FAILED( hr ) ){
        DEBUG_INFO("Can’t build the graph");
        return hr;
    }

    hr = m_pGrabber->GetConnectedMediaType( &mt );
    if ( FAILED( hr) ){
        DEBUG_INFO("Failt to read the connected media type");
        return hr;
    }

    // Save sample info
    VIDEOINFOHEADER * vih = (VIDEOINFOHEADER*) mt.pbFormat;
    vsi.m_biCompression = vih->bmiHeader.biCompression;
    vsi.m_biBitCount = vih->bmiHeader.biBitCount;
    vsi.m_nWidth = vih->bmiHeader.biWidth;
    vsi.m_nHeight = vih->bmiHeader.biHeight;
    vsi.m_AvgTimePerFrame = vih->AvgTimePerFrame;

    FreeMediaType(mt);

    return S_OK;
}

HRESULT VideoCapture::StartPreview(int iDeviceID,
                                   HWND hWnd,
                                   VideoFrmQueue* pQueue
                                   )
{
    DEBUG_INFO("VideoCapture::StartPreview()......");

    HRESULT hr;
    hr = InitCaptureGraphBuilder();
    if (FAILED(hr)){
        DEBUG_INFO("Failed to get video interfaces!");
        return hr;
    }
    // Bind Device Filter. We know the device because the id was passed in
    if(!BindFilter(iDeviceID, &m_pBF))return S_FALSE;
    hr = m_pGB->AddFilter(m_pBF, L"Capture Filter");

    //TODO: test for frm rate, etc
    hr = GetCaptureRatio(m_pBF, m_pCapture);
    //hr = SetCaptureRatio(m_pBF, m_pCapture, CAPTURE_WIDTH, CAPTURE_HEIGHT, 10);
    //hr = GetCaptureRatio(m_pBF, m_pCapture);


    //hr = m_pCapture->RenderStream(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video, 
    // m_pBF, NULL, NULL);
    // create a sample grabber
    //hr = CoCreateInstance( CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER, IID_ISampleGrabber, (void**)&m_pGrabber );
    m_pGrabber.CoCreateInstance(CLSID_SampleGrabber);
    if(FAILED(hr)){
        DEBUG_INFO("Fail to create SampleGrabber, maybe qedit.dll is not registered?");
        return hr;
    }
    CComQIPtr< IBaseFilter, &IID_IBaseFilter > pGrabBase( m_pGrabber );
    // set video format
    //AM_MEDIA_TYPE mt;
    CMediaType mt;
    ZeroMemory(&mt, sizeof(AM_MEDIA_TYPE));
    mt.majortype = MEDIATYPE_Video;
    mt.subtype = CAPTURE_SUBTYPE;
    /*
    //// Work out the GUID for the subtype from the header info.
    //const GUID SubTypeGUID = GetBitmapSubtype(&pvi->bmiHeader);
    //mt.SetSubtype(&SubTypeGUID);
    //mt.SetSampleSize(pvi->bmiHeader.biSizeImage);
    */

    hr = m_pGrabber->SetMediaType(&mt);

    if( FAILED(hr) ){
        DEBUG_INFO("Fail to set media type!");
        return hr;
    }
    //TODO: test for frm rate, etc
    //hr = GetCaptureRatio(m_pBF, m_pCapture);
    //hr = SetCaptureRatio(m_pBF, m_pCapture, CAPTURE_WIDTH, CAPTURE_HEIGHT, 10);


    hr = m_pGB->AddFilter( pGrabBase, L"Grabber" );
    if( FAILED( hr ) ){
        DEBUG_INFO("Fail to put sample grabber in graph");
        return hr;
    }

    // try to render preview/capture pin
    hr = m_pCapture->RenderStream(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video,m_pBF,pGrabBase,NULL);
    if( FAILED( hr ) ){
        DEBUG_INFO("Can’t build the graph");
        return hr;
    }

    hr = m_pGrabber->GetConnectedMediaType( &mt );
    if ( FAILED( hr) ){
        DEBUG_INFO("Failed to read the connected media type");
        return hr;
    }

    //// Save sample info
    //VIDEOINFOHEADER * vih = (VIDEOINFOHEADER*) mt.pbFormat;
    //vsi.m_biCompression = vih->bmiHeader.biCompression;
    //vsi.m_biBitCount = vih->bmiHeader.biBitCount;
    //vsi.m_nWidth = vih->bmiHeader.biWidth;
    //vsi.m_nHeight = vih->bmiHeader.biHeight;
    //vsi.m_AvgTimePerFrame = vih->AvgTimePerFrame;

    FreeMediaType(mt);

    hr = m_pGrabber->SetBufferSamples( FALSE );
    if (FAILED(hr)) return hr;
    hr = m_pGrabber->SetOneShot( FALSE );
    if (FAILED(hr)) return hr;
    m_sampleCB.SetQueue(pQueue);
    hr = m_pGrabber->SetCallback( &m_sampleCB, 1 );
    if (FAILED(hr)) return hr;

    // setup window for video capture
    m_hWnd = hWnd;
    hr = SetupVideoWindow();
    if (FAILED(hr)) return hr;

    // resume capturing
    hr = m_pMC->Run();
    if(FAILED(hr))
    {
        DEBUG_INFO("Couldn’t run the graph!");
        DebugBreak();
    }

    DEBUG_INFO("VideoCapture::StartPreview() DONE");

    return hr;
}

// bind filter to device
bool VideoCapture::BindFilter(int deviceId, IBaseFilter **pFilter)
{
    if (deviceId < 0) return false;

    // enumerate all video capture devices

    ICreateDevEnum *pCreateDevEnum;

    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, 
        CLSCTX_INPROC_SERVER,
        IID_ICreateDevEnum,
        (void**)&pCreateDevEnum);
    if (hr != NOERROR) return false;

    CComPtr<IEnumMoniker> pEm;

    hr = pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory,&pEm, 0);
    if (hr != NOERROR) return false;
    pEm->Reset();
    ULONG cFetched;
    CComPtr<IMoniker> pM;
    int index = 0;
    while(hr = pEm->Next(1, &pM, &cFetched), hr==S_OK, index <= deviceId)
    {
        IPropertyBag *pBag;
        hr = pM->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pBag);
        if(SUCCEEDED(hr)) 
        {
            VARIANT var;
            var.vt = VT_BSTR;
            hr = pBag->Read(L"FriendlyName", &var, NULL);
            if (hr == NOERROR) 
            {
                if (index == deviceId)
                {
                    pM->BindToObject(0, 0, IID_IBaseFilter, (void**)pFilter);
                }
                SysFreeString(var.bstrVal);
            }   
            pBag=NULL;
        }
        pM=NULL;
        index++;
    }
    return true;
}

// create filters, and enumerate iterfaces
HRESULT VideoCapture::InitCaptureGraphBuilder()
{
    HRESULT hr;

    // create IGraphBuilder
    hr = CoCreateInstance(CLSID_FilterGraph, NULL, 
        CLSCTX_INPROC_SERVER,
        IID_IGraphBuilder, (void **)&m_pGB);
    if (FAILED(hr)) return hr;

    // create ICaptureGraphBuilder2
    hr = CoCreateInstance(CLSID_CaptureGraphBuilder2 , NULL,
        CLSCTX_INPROC,
        IID_ICaptureGraphBuilder2, (void **)&m_pCapture);
    if (FAILED(hr)) return hr;

    // save IGraphBuilder
    m_pCapture->SetFiltergraph(m_pGB);

    // query IMediaControl
    hr = m_pGB->QueryInterface(IID_IMediaControl, (void **)&m_pMC);
    if (FAILED(hr)) return hr;
    // query IVideoWindow
    hr = m_pGB->QueryInterface(IID_IVideoWindow, (LPVOID *) &m_pVW);
    if (FAILED(hr)) return hr;
    return hr;

}

// setup video window characteristics
HRESULT VideoCapture::SetupVideoWindow()
{
    HRESULT hr;

    hr = m_pVW->put_Visible(OAFALSE);

    hr = m_pVW->put_Owner((OAHWND)m_hWnd);
    if (FAILED(hr)) return hr;

    hr = m_pVW->put_WindowStyle(WS_CHILD | WS_CLIPCHILDREN);
    if (FAILED(hr)) return hr;

    ResizeVideoWindow();
    hr = m_pVW->put_Visible(OATRUE);
    return hr;
}

void VideoCapture::ResizeVideoWindow()
{
    if (m_pVW)
    {
        // fill entire window with image
        CRect rc;
        ::GetClientRect(m_hWnd,&rc);
        m_pVW->SetWindowPosition(0, 0, rc.right, rc.bottom);
    } 
}

// config for camera data source format (resolution, RGB/I420, etc.)
void VideoCapture::ConfigCameraPin(HWND hwndParent)
{
    HRESULT hr;
    IAMStreamConfig *pSC;
    ISpecifyPropertyPages *pSpec;

    // need to stop before setting property page of pin
    m_pMC->Stop();

    hr = m_pCapture->FindInterface(&PIN_CATEGORY_CAPTURE,
        &MEDIATYPE_Video, m_pBF,
        IID_IAMStreamConfig, (void **)&pSC);

    CAUUID cauuid;

    hr = pSC->QueryInterface(IID_ISpecifyPropertyPages, (void **)&pSpec);
    if(hr == S_OK)
    {
        hr = pSpec->GetPages(&cauuid);
        // show property page
        hr = OleCreatePropertyFrame(hwndParent, 30, 30, NULL, 1, 
            (IUnknown **)&pSC, cauuid.cElems,
            (GUID *)cauuid.pElems, 0, 0, NULL);

        // release memory and resource
        CoTaskMemFree(cauuid.pElems);
        pSpec->Release();
        pSC->Release();
    }
    // resume capturing
    m_pMC->Run();
}

// config filters for brightness, chroma, saturation, etc.
void VideoCapture::ConfigCameraFilter(HWND hwndParent)
{
    HRESULT hr=0;

    ISpecifyPropertyPages *pProp;

    hr = m_pBF->QueryInterface(IID_ISpecifyPropertyPages, (void **)&pProp);

    if (SUCCEEDED(hr)) 
    {
        // get filter info
        FILTER_INFO FilterInfo;
        hr = m_pBF->QueryFilterInfo(&FilterInfo); 
        IUnknown *pFilterUnk;
        m_pBF->QueryInterface(IID_IUnknown, (void **)&pFilterUnk);

        // display filter property
        CAUUID caGUID;
        pProp->GetPages(&caGUID);

        OleCreatePropertyFrame(
            hwndParent,
            0, 0,                   // Reserved
            FilterInfo.achName,     // Dialog Title
            1,                      // targets count of this filter
            &pFilterUnk,            // targets pointer array
            caGUID.cElems,          // count of property pages
            caGUID.pElems,          // CLSID array of property pages
            0,                      // local id
            0, NULL                 // Reserved
            );

        // release memory, resource
        CoTaskMemFree(caGUID.pElems);
        pFilterUnk->Release();
        FilterInfo.pGraph->Release(); 
        pProp->Release();
    }
}

HRESULT VideoCapture::StopPreview()
{
    DEBUG_INFO("VideoCapture::StopPreview()......");
    HRESULT hr = m_pMC->Stop();
    m_pVW->put_Visible(OAFALSE);
    m_pVW->put_Owner(NULL);
    DEBUG_INFO("VideoCapture::StopPreview() DONE");
    return hr;
}