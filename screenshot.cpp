
#define _CRT_SECURE_NO_WARNINGS

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <wincodec.h>
#include <stdio.h>
#include <shellapi.h>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <sstream>

#pragma comment(lib, "windowscodecs.lib")


wchar_t file_prefix[MAX_PATH]=L"monitor_";
wchar_t file_time_prefix[MAX_PATH] = L"";

struct MonitorData
{
    int  index   = 0;
    int  success = 0;
    int  failed  = 0;
    bool run_ms_paint = false;
    bool mask[100];
};

bool save_bmp(
    void* pixels,
    int w,
    int h,
    const wchar_t* filename)
{
    DWORD imageSize = (DWORD)w * (DWORD)h * 4;

    BITMAPFILEHEADER bfh = {};
    bfh.bfType = 0x4D42; // "BM"
    bfh.bfOffBits =
        sizeof(BITMAPFILEHEADER) +
        sizeof(BITMAPINFOHEADER);
    bfh.bfSize = bfh.bfOffBits + imageSize;

    BITMAPINFOHEADER bih = {};
    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = w;
    bih.biHeight = h;       // bottom-up
    bih.biPlanes = 1;
    bih.biBitCount = 32;
    bih.biCompression = BI_RGB;

    FILE* file = NULL;

    if (_wfopen_s(&file, filename, L"wb") != 0 || !file)
        return false;

    bool ok =
        fwrite(&bfh, sizeof(bfh), 1, file) == 1 &&
        fwrite(&bih, sizeof(bih), 1, file) == 1 &&
        fwrite(pixels, imageSize, 1, file) == 1;

    fclose(file);

    return ok;
}

bool save_png(
    void* pixels,
    int w,
    int h,
    const wchar_t* filename)
{
    HRESULT hr;

    // WIC initialization
    IWICImagingFactory* factory = NULL;

    hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&factory));

    if (FAILED(hr))
        return false;

    IWICBitmapEncoder* encoder = NULL;

    hr = factory->CreateEncoder(
        GUID_ContainerFormatPng,
        NULL,
        &encoder);

    if (FAILED(hr))
    {
        factory->Release();
        return false;
    }

    IWICStream* stream = NULL;

    hr = factory->CreateStream(&stream);

    if (FAILED(hr))
    {
        encoder->Release();
        factory->Release();
        return false;
    }

    hr = stream->InitializeFromFilename(
        filename,
        GENERIC_WRITE);

    if (FAILED(hr))
    {
        stream->Release();
        encoder->Release();
        factory->Release();
        return false;
    }

    hr = encoder->Initialize(
        stream,
        WICBitmapEncoderNoCache);

    if (FAILED(hr))
    {
        stream->Release();
        encoder->Release();
        factory->Release();
        return false;
    }

    IWICBitmapFrameEncode* frame = NULL;
    IPropertyBag2* props = NULL;

    hr = encoder->CreateNewFrame(
        &frame,
        &props);

    if (FAILED(hr))
    {
        stream->Release();
        encoder->Release();
        factory->Release();
        return false;
    }

    if (props)
        props->Release();

    hr = frame->Initialize(NULL);

    if (FAILED(hr))
    {
        frame->Release();
        stream->Release();
        encoder->Release();
        factory->Release();
        return false;
    }

    hr = frame->SetSize(w, h);

    if (FAILED(hr))
    {
        frame->Release();
        stream->Release();
        encoder->Release();
        factory->Release();
        return false;
    }

    WICPixelFormatGUID format = GUID_WICPixelFormat32bppBGRA;

    hr = frame->SetPixelFormat(&format);

    if (FAILED(hr))
    {
        frame->Release();
        stream->Release();
        encoder->Release();
        factory->Release();
        return false;
    }

    DWORD stride = (DWORD)w * 4;
    DWORD imageSize = stride * (DWORD)h;

    hr = frame->WritePixels(
        h,
        stride,
        imageSize,
        (BYTE*)pixels);

    if (SUCCEEDED(hr))
        hr = frame->Commit();

    if (SUCCEEDED(hr))
        hr = encoder->Commit();

    frame->Release();
    stream->Release();
    encoder->Release();
    factory->Release();

    return SUCCEEDED(hr);
}

bool save_screenshot(
    int x,
    int y,
    int w,
    int h,
    const wchar_t* bmpFilename,
    const wchar_t* pngFilename)
{
    HDC hScreenDC = GetDC(NULL);

    if (!hScreenDC)
        return false;

    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);

    if (!hMemoryDC)
    {
        ReleaseDC(NULL, hScreenDC);
        return false;
    }

    BITMAPINFO bmi = {};

    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h;       // bottom-up
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pixels = NULL;

    HBITMAP hBitmap = CreateDIBSection(
        hMemoryDC,
        &bmi,
        DIB_RGB_COLORS,
        &pixels,
        NULL,
        0);

    if (!hBitmap)
    {
        DeleteDC(hMemoryDC);
        ReleaseDC(NULL, hScreenDC);
        return false;
    }

    HGDIOBJ oldBitmap =
        SelectObject(hMemoryDC, hBitmap);

    BOOL result = BitBlt(
        hMemoryDC,
        0,
        0,
        w,
        h,
        hScreenDC,
        x,
        y,
        SRCCOPY | CAPTUREBLT);

    SelectObject(hMemoryDC, oldBitmap);

    bool bmpOK = false;
    bool pngOK = false;

    if (result)
    {
        bmpOK = true;
        /*bmpOK = save_bmp(
            pixels,
            w,
            h,
            bmpFilename);*/
        pngOK = true;
        pngOK = save_png(
            pixels,
            w,
            h,
            pngFilename);


    }

    DeleteObject(hBitmap);
    DeleteDC(hMemoryDC);
    ReleaseDC(NULL, hScreenDC);

    return bmpOK && pngOK;
}

BOOL CALLBACK MonitorEnumProc(
    HMONITOR hMonitor,
    HDC,
    LPRECT,
    LPARAM lParam)
{
    MonitorData* data =
        (MonitorData*)lParam;

    MONITORINFO mi = {};
    mi.cbSize = sizeof(mi);

    if (!GetMonitorInfoW(hMonitor, &mi))
    {
        data->failed++;
        data->index++;
        return TRUE;
    }

    int x = mi.rcMonitor.left;
    int y = mi.rcMonitor.top;

    int w =
        mi.rcMonitor.right -
        mi.rcMonitor.left;

    int h =
        mi.rcMonitor.bottom -
        mi.rcMonitor.top;

    wchar_t bmpFilename[MAX_PATH];
    wchar_t pngFilename[MAX_PATH];

    if (data->index<100 && data->mask[data->index])
    {
        swprintf_s(
            bmpFilename,
            _countof(bmpFilename),
            L"%s%s_%d.bmp",
            file_prefix,
            file_time_prefix,
            data->index);

        swprintf_s(
            pngFilename,
            _countof(pngFilename),
            L"%s%s_%d.png",
            file_prefix,
            file_time_prefix,
            data->index);

        if (save_screenshot(
            x,
            y,
            w,
            h,
            bmpFilename,
            pngFilename))
        {
            data->success++;

            if (data->run_ms_paint) {

                ShellExecuteW(
                    NULL,
                    L"open",
                    L"mspaint.exe",
                    pngFilename,
                    NULL,
                    SW_SHOWNORMAL
                );
            }
        }
        else
        {
            data->failed++;
        }
    }
    data->index++;

    return TRUE;
}

int WINAPI WinMain(
    HINSTANCE,
    HINSTANCE,
    LPSTR,
    int)
{
    MonitorData data = {};
    bool mask_is_cleaned = false;
    for (int i = 0; i < sizeof(data.mask); i++) {
        data.mask[i] = 1;
    }

    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc); 
    if (!argv) { MessageBoxW(NULL, L"CommandLineToArgvW() failed.", L"screenshot", MB_OK | MB_ICONERROR); return 1; }
    

   

    //file_prefix[0] = 0;

    for (int i = 1; i < argc; i++) {

        if (wcscmp(argv[i], L"-m")==0) 
        {
            if (i + 1 < argc) {

                if (!mask_is_cleaned) {
                    mask_is_cleaned = true;
                    for (int i = 0; i < sizeof(data.mask); i++) {
                        data.mask[i] = 0;
                    }
                }
                i++;

                int idx = _wtoi(argv[i]);
                if (idx < 100)
                    data.mask[idx] = 1;
            }
        }
        
        if (wcscmp(argv[i], L"-p") == 0) {
            data.run_ms_paint = true;
        }


        if (wcscmp(argv[i], L"-t") == 0)
        {
            auto t   =   std::time(nullptr);
            auto lt  =  *std::localtime(&t);

            std::ostringstream oss;
            oss << std::put_time(&lt, "%Y-%m-%d_%H-%M-%S");
            auto str = oss.str();
            std::wstring ws(str.begin(), str.end());

            swprintf_s(
                file_time_prefix,
                _countof(file_prefix),
                L"%s",
                ws.c_str());
             
        }

        if (wcscmp(argv[i], L"-o")==0)
        {
            if (i + 1 < argc) {
                i++;
                swprintf_s(
                    file_prefix,
                    _countof(file_prefix),
                    L"%s",
                    argv[i]);
                
            }
        }

        if (
            wcscmp(argv[i], L"-h") == 0
            ||
            wcscmp(argv[i], L"-help") == 0
            ||
            wcscmp(argv[i], L"--help") == 0
            ) {
            MessageBoxW(
               NULL,
               L"Usage: screenshot.exe [-m 0 -m 1 ...] [-o file_prefix] [--help|-h] [-t]\n"
               L" By default it saves screenshots from monitors into separate files\n"
               L"-m monitor number, for example -m 0 -m 2 \n"
               L"-o output file prefix for example c:\\temp\\screenshot \n"
               L"-t add date-time to file name\n"
               L"-p run mspaint.exe\n"
               L"-h help\n"
               L"Example screenshot.exe -m 0 ",
               L"Screenshot",
               MB_OK | MB_ICONINFORMATION);

        }
    }


    // Important for 125%, 150%, 200%, etc.
    if (!SetProcessDpiAwarenessContext(
        DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
    {
        SetProcessDpiAwarenessContext(
            DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
    }

    // WIC / COM
    HRESULT hr = CoInitializeEx(
        NULL,
        COINIT_MULTITHREADED);

    if (FAILED(hr))
    {
        MessageBoxW(
            NULL,
            L"CoInitializeEx failed",
            L"Screenshot",
            MB_OK | MB_ICONERROR);

        return 1;
    }

    

    EnumDisplayMonitors(
        NULL,
        NULL,
        MonitorEnumProc,
        (LPARAM)&data);

    CoUninitialize();
    /*
    wchar_t message[256];

    swprintf_s(
        message,
        L"Monitors: %d\n"
        L"Saved: %d\n"
        L"Failed: %d",
        data.index,
        data.success,
        data.failed);

  MessageBoxW(
        NULL,
        message,
        L"Screenshot",
        MB_OK | MB_ICONINFORMATION);*/

    return data.failed ? 1 : 0;
}

