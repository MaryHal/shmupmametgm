#include "tgm_util.h"
#include <stdio.h>

#if defined(_WIN64) || defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static WCHAR filepath[256];
static const char* sharedMemFilename = "taptracker_data";
static HANDLE fileHandle;
static HANDLE mapping;
static LPVOID view;

static void messageBoxWithLastError()
{
    static const size_t BUFFER_SIZE = 256;

    WCHAR buf[BUFFER_SIZE];
    FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf, BUFFER_SIZE, NULL);
    MessageBox(NULL, buf, L"Hello", MB_OK);
}

void memorymap_create(size_t dataSize)
{
    swprintf(filepath, 256, L"%s\\%s", getenv("APPDATA"), sharedMemFilename);

    fileHandle = CreateFile(
        filepath,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (fileHandle == INVALID_HANDLE_VALUE)
    {
        MessageBox(NULL, L"CreateFile has failed", L"Hello", MB_OK);
        messageBoxWithLastError();
        exit(1);
    }

    mapping = CreateFileMapping(
        fileHandle,
        NULL,
        PAGE_READWRITE,
        0,
        dataSize,
        NULL);

    if (mapping == NULL)
    {
        MessageBox(NULL, L"CreateFileMapping has failed", L"Hello", MB_OK);
        messageBoxWithLastError();
        exit(1);
    }

    view = MapViewOfFile(
        mapping,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        dataSize);

    if (view == NULL)
    {
        MessageBox(NULL, L"MapViewOfFile has failed", L"Hello", MB_OK);
        messageBoxWithLastError();
        exit(1);
    }
}

void memorymap_destroy()
{
    if (UnmapViewOfFile(view)    == 0 ||
        CloseHandle(mapping)     == 0 ||
        CloseHandle(fileHandle)  == 0 ||
        DeleteFile(filepath) == 0)
    {
        messageBoxWithLastError();
    }
}

void* memorymap_getPointer()
{
    return view;
}

int createDir(const char* path)
{
    const WCHAR* wcpath;
    int nChars = MultiByteToWideChar(CP_ACP, 0, path, -1, NULL, 0);
    wcpath = (WCHAR*)malloc(sizeof(WCHAR) * nChars);
    MultiByteToWideChar(CP_ACP, 0, path, -1, (LPWSTR)wcpath, nChars);

    if (GetFileAttributes(wcpath) == INVALID_FILE_ATTRIBUTES)
    {
        if (CreateDirectory(wcpath, NULL) == 0)
        {
            messageBoxWithLastError();
        }
    }

    free((void*)wcpath);

    return 0;
}

#endif
