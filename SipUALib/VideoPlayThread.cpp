#include "StdAfx.h"
#include "VideoPlayThread.h"
#include "PushSource.h"



IMPLEMENT_DYNCREATE(VideoPlayThread, VideoPlayThread::super)

VideoPlayThread::VideoPlayThread(void)
{
    m_hWnd = NULL;
    m_pQueue = NULL;
    m_renderer = NULL;

    DEBUG_INFO("VideoPlayThread::VideoPlayThread() DONE");
}


VideoPlayThread::~VideoPlayThread(void)
{
    DEBUG_INFO("VideoPlayThread::~VideoPlayThread() DONE");
}

BOOL VideoPlayThread::CreateThread(VideoFrmQueue* pQueue)
{
    m_pQueue = pQueue;
    return super::CreateThread();
}

//HRES report helper
void PrintHResErr(LPCSTR info, HRESULT hr)
{
    SPRINTF_S(dbg_str,
        "%s: [%X]"
        , info, hr);
    DEBUG_INFO(dbg_str);
}

HRESULT VideoPlayThread::Play(VideoFrmQueue* pQueue, HWND hWnd)
{
    HRESULT hr = S_OK;

    CoInitialize(NULL);

    do 
    {
        IUnknown *pUnk = NULL;
        m_renderer = CPushSourceBitmap::CreateInstance(pUnk, &hr);

        if(FAILED(hr) || m_renderer == NULL)
        {
            SPRINTF_S(dbg_str,
                DBG_STR_SIZE,
                "Could not create filter - HRESULT 0x%8.8X\n",
                hr);
            DEBUG_INFO(dbg_str);
            break;
        }

        ((CPushSourceBitmap*)m_renderer)->SetQueue(pQueue);

        IFilterGraph *pFG = NULL;
        hr = SelectAndRender((CPushSourceBitmap*)m_renderer, &pFG);

        if(FAILED(hr))
        {
            SPRINTF_S(dbg_str,
                DBG_STR_SIZE,
                "Failed to create graph and render file - HRESULT 0x%8.8X",
                hr);
            DEBUG_INFO(dbg_str);
            break;
        }
        else
        {
            //  Play
            hr = PlayBufferWait(pFG, hWnd);
            if(FAILED(hr))
            {
                SPRINTF_S(dbg_str,
                    DBG_STR_SIZE,
                    "Failed to play graph - HRESULT 0x%8.8X",
                    hr);
                DEBUG_INFO(dbg_str);
                break;
            }
        }

        if(pFG)
        {
            ULONG ulRelease = pFG->Release();
            if(ulRelease != 0)
            {
                SPRINTF_S(dbg_str,
                    DBG_STR_SIZE,
                    "Filter graph count not 0! (was %d)",
                    ulRelease);
                DEBUG_INFO(dbg_str);
                break;
            }
        }
    } while (0);

    CoUninitialize();
    return hr;
}


//  Select a filter into a graph and render its output pin,
//  returning the graph

HRESULT VideoPlayThread::SelectAndRender(CSource *pReader, IFilterGraph **ppFG)
{
    CheckPointer(pReader,E_POINTER);
    CheckPointer(ppFG,E_POINTER);

    HRESULT hr = S_OK;

    do 
    {
        /*  Create filter graph */
        hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC,
            IID_IFilterGraph, (void**) ppFG);
        if(FAILED(hr))
        {
            PrintHResErr("[Direct show] Failed to create filter graph", hr);
            break;
        }

        /*  Add our filter */
        hr = (*ppFG)->AddFilter(pReader, NULL);

        if(FAILED(hr))
        {
            PrintHResErr("[Direct show] Failed to add our filter", hr);
            break;
        }

        /*  Get a GraphBuilder interface from the filter graph */
        IGraphBuilder *pBuilder;

        hr = (*ppFG)->QueryInterface(IID_IGraphBuilder, (void **)&pBuilder);
        if(FAILED(hr))
        {
            PrintHResErr("[Direct show] Failed to get GraphBuilder from the filter graph", hr);
            break;
        }

        /*  Render our output pin */
        hr = pBuilder->Render(pReader->GetPin(0));
        if(FAILED(hr))
        {
            PrintHResErr("[Direct show] Failed to render our output pin", hr);
            break;
        }

        /* Release interface and return */
        pBuilder->Release();

    } while (0);

    return hr;
}

HRESULT VideoPlayThread::PlayBufferWait(IFilterGraph *pFG, HWND hWnd)
{
    DEBUG_INFO("VideoPlayThread::PlayBufferWait...");

    CheckPointer(pFG,E_POINTER);

    HRESULT hr;
    IMediaControl *pMC=0;
    IMediaEvent   *pME=0;
    IVideoWindow  *pVW=0;
    pVW=0;

    do 
    {
        hr = pFG->QueryInterface(IID_IMediaControl, (void **)&pMC);
        if(FAILED(hr))
        {
            PrintHResErr("[Direct show] Failed to query MediaControl", hr);
            break;
        }

        hr = pFG->QueryInterface(IID_IMediaEvent, (void **)&pME);
        if(FAILED(hr))
        {
            PrintHResErr("[Direct show] Failed to query MediaEvent", hr);
            pMC->Release();
            break;
        }

        // query IVideoWindow
        hr = pFG->QueryInterface(IID_IVideoWindow, (LPVOID *) &pVW);
        if(FAILED(hr))
        {
            PrintHResErr("[Direct show] Failed to query VideoWindow", hr);
            pVW->Release();
            break;
        }
        hr = pVW->put_Visible(OAFALSE);

        hr = pVW->put_Owner((OAHWND)hWnd);
        if (FAILED(hr)) return hr;

        hr = pVW->put_WindowStyle(WS_CHILD | WS_CLIPCHILDREN);
        if (FAILED(hr)) return hr;

        hr = pVW->put_Visible(OATRUE);
        if (FAILED(hr)) return hr;

        //ResizeVideoWindow();
        CRect rc;
        ::GetClientRect(hWnd,&rc);
        pVW->SetWindowPosition(0, 0, rc.right, rc.bottom);

        OAEVENT oEvent;
        hr = pME->GetEventHandle(&oEvent);
        if (FAILED(hr))
        {
            PrintHResErr("[Direct show] Failed to get eventHandle", hr);
            break;
        }
        else
        {
            hr = pMC->Run();

            if (FAILED(hr))
            {
                PrintHResErr("[Direct show] Failed to run medie control", hr);
                break;
            }
            else
            {
                LONG levCode;
                hr = pME->WaitForCompletion(INFINITE, &levCode);
                if (FAILED(hr))
                {
                    PrintHResErr("[Direct show] Failed to WaitForCompletion", hr);
                }


                hr = pMC->Stop();
                ASSERT(SUCCEEDED(hr));
                if (FAILED(hr))
                {
                    PrintHResErr("[Direct show] Failed to stop media control", hr);
                }

                hr = pFG->RemoveFilter((CPushSourceBitmap*)m_renderer);
                if(FAILED(hr))
                {
                    SPRINTF_S(dbg_str,
                        DBG_STR_SIZE,
                        "Failed to RemoveFilter - HRESULT 0x%8.8X",
                        hr);
                    DEBUG_INFO(dbg_str);
                }

                pVW->put_Visible(OAFALSE);
                pVW->put_Owner(NULL);
                pVW->Release();
            }
        }

        pMC->Release();
        pME->Release();

    } while (0);

    DEBUG_INFO("VideoPlayThread::PlayBufferWait DONE");

    return hr;
}

void VideoPlayThread::DoJob()
{
    DEBUG_INFO("VideoPlayThread::DoJob()......");

    Play(m_pQueue, m_hWnd);

    DEBUG_INFO(L"VideoPlayThread::DoJob() DONE");
}

void VideoPlayThread::Stop()
{
    DEBUG_INFO("VideoPlayThread::Stop()......");

    super::Stop();

    ((CPushSourceBitmap*)m_renderer)->StopPlay();

    //TODO: if no video frames to play (remote has no camera or doesn't use it),
    //      we have to limit wait time here, since WaitForComplete(INFINITE) never stopped.
    //      This maybe caused by failure in PlayBufferWait(). Need to see to it later.
    // Must wait for FilterGraph to complete
    // before destruction of this thread object.
    // Otherwise, FilterGraph->Release will fall
    // since FilterGraph is not a valid pointer.
    this->WaitForComplete(2000);

    DEBUG_INFO(L"VideoPlayThread::Stop() DONE");
}