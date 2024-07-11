#include <Windows.h>
#include <cstdio>
#include <detours.h>
#include <handleapi.h>
#include <string>
#include <memory>
#include <inttypes.h>
#include <winnt.h>
#include "producer.h"

using namespace std;

CRITICAL_SECTION SyncSection;
std::shared_ptr<Producer> g_producer = nullptr;

std::string string_format(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    size_t len = std::vsnprintf(nullptr, 0, format, args);
    va_end(args);
    std::unique_ptr<char[]> buf(new char[len + 1]);
    va_start(args, format);
    std::vsnprintf(buf.get(), len + 1, format, args);
    va_end(args);
    return std::string(buf.get(), buf.get() + len);
}

static auto Real_HeapAlloc = ::HeapAlloc;
static auto Real_HeapFree = ::HeapFree;
static char g_buffer[1024] = {0};

// Detoured HeapAlloc API function.
static LPVOID WINAPI HeapAllocHook (HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes)
{
    LPVOID rv = Real_HeapAlloc (hHeap, dwFlags, dwBytes);

    EnterCriticalSection (&SyncSection);
    if (g_producer != nullptr)
    {
        sprintf(g_buffer, "HeapAlloc: hHeap=0x%" PRIx64 " dwBytes=%llu", (uintptr_t)rv, dwBytes); 
        g_producer->Send(g_buffer);
    }
    LeaveCriticalSection (&SyncSection);

    return rv;
}

BOOL WINAPI HeapFreeHook (HANDLE hHeap, DWORD dwFlags, LPVOID lpMem)
{
    BOOL result = Real_HeapFree (hHeap, dwFlags, lpMem);

    EnterCriticalSection (&SyncSection);
    if (g_producer != nullptr)
    {
        sprintf(g_buffer, "HeapFree:  hHeap=0x%" PRIx64, (uintptr_t)lpMem);
        g_producer->Send(g_buffer);
    }
    LeaveCriticalSection (&SyncSection);

    return result;
}

//-------------------------------------------------------------------------
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID reserved)
{
    LONG error;
    UNREFERENCED_PARAMETER(hinst);
    UNREFERENCED_PARAMETER(reserved);

    if (DetourIsHelperProcess())
        return TRUE;

    if (dwReason == DLL_PROCESS_ATTACH)
    {
        InitializeCriticalSection (&SyncSection);
        g_producer = std::make_shared<Producer>();
        DetourRestoreAfterWith();
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach(&(PVOID&)Real_HeapAlloc, HeapAllocHook);
        DetourAttach(&(PVOID&)Real_HeapFree, HeapFreeHook);
        error = DetourTransactionCommit();
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourDetach(&(PVOID&)Real_HeapAlloc, HeapAllocHook);
        DetourDetach(&(PVOID&)Real_HeapFree, HeapFreeHook);
        error = DetourTransactionCommit();

        DeleteCriticalSection (&SyncSection);
        g_producer = nullptr;
    }
    return TRUE;
}
