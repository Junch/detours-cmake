#include <windows.h>
#include <iostream>
#include <tlhelp32.h>
#include <string.h>

// https://medium.com/@s12deff/persistent-api-hooking-with-detours-via-dll-injection-30a38346aa4c#/

using namespace std;

bool IsProcessRunning( const string &processName ) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
    if ( hSnapshot == INVALID_HANDLE_VALUE ) return false;
    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof( PROCESSENTRY32 );
    if ( Process32First( hSnapshot, &processEntry ) ) {
        do {
            if ( processName.compare( processEntry.szExeFile ) == 0 ) {
                CloseHandle( hSnapshot );
                return true;
            }
        } while ( Process32Next( hSnapshot, &processEntry ) );
    }
    CloseHandle( hSnapshot );
    return false;
}

bool IsDLLLoaded( DWORD processId, const std::wstring &dllName ) {
    HANDLE hModuleSnap = CreateToolhelp32Snapshot(
        TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processId );
    if ( hModuleSnap == INVALID_HANDLE_VALUE ) {
        return false;
    }

    MODULEENTRY32W moduleEntry;
    moduleEntry.dwSize = sizeof( MODULEENTRY32W );
    bool dllFound      = false;
    if ( Module32FirstW( hModuleSnap, &moduleEntry ) ) {
        do {
            if ( lstrcmpiW( dllName.c_str(), moduleEntry.szModule ) == 0 ) {
                dllFound = true;
                break;
            }
        } while ( Module32NextW( hModuleSnap, &moduleEntry ) );
    }
    CloseHandle( hModuleSnap );
    return dllFound;
}

DWORD GetPIDbyProcName( const std::wstring &procName ) {
    DWORD  pid   = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
    if ( hSnap == INVALID_HANDLE_VALUE ) {
        return 0; // 返回一个错误代码，表示没有找到快照
    }

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof( PROCESSENTRY32W );
    if ( Process32FirstW( hSnap, &pe32 ) ) {
        do {
            if ( lstrcmpiW( pe32.szExeFile, procName.c_str() ) == 0 ) {
                pid = pe32.th32ProcessID;
                break;
            }
        } while ( Process32NextW( hSnap, &pe32 ) );
    }
    CloseHandle( hSnap );
    return pid;
}

bool InjectDll( DWORD pid, const std::wstring &dllPath ) {
    HANDLE hProc = OpenProcess( PROCESS_ALL_ACCESS, FALSE, pid );
    if ( hProc == NULL ) {
        cerr << "OpenProcess() failed: " << GetLastError() << endl;
        return false;
    }

    HMODULE hKernel32 = GetModuleHandle( "Kernel32" );
    FARPROC lb        = GetProcAddress( hKernel32, "LoadLibraryW" );
    SIZE_T  allocSize =
        ( dllPath.size() + 1 ) * sizeof( wchar_t ); // 计算宽字符字符串的大小
    LPVOID allocMem = VirtualAllocEx(
        hProc, NULL, allocSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE );
    if ( allocMem == NULL ) {
        cerr << "VirtualAllocEx() failed: " << GetLastError() << endl;
        CloseHandle( hProc );
        return false;
    }

    if ( !WriteProcessMemory( hProc, allocMem, &dllPath[ 0 ], allocSize,
                              NULL ) ) {
        cerr << "WriteProcessMemory() failed: " << GetLastError() << endl;
        VirtualFreeEx( hProc, allocMem, allocSize, MEM_RELEASE );
        CloseHandle( hProc );
        return false;
    }

    HANDLE rThread = CreateRemoteThread(
        hProc, NULL, 0, (LPTHREAD_START_ROUTINE) lb, allocMem, 0, NULL );
    if ( rThread == NULL ) {
        cerr << "CreateRemoteThread() failed: " << GetLastError() << endl;
        VirtualFreeEx( hProc, allocMem, allocSize, MEM_RELEASE );
        CloseHandle( hProc );
        return false;
    }
    WaitForSingleObject( rThread, INFINITE);

    VirtualFreeEx( hProc, allocMem, allocSize, MEM_RELEASE );
    CloseHandle( rThread );
    CloseHandle( hProc );
    return true;
}

// Remove the injected Dll
// Reverse order of injection
BOOL EjectDll(DWORD dwPID, LPCWSTR szDllPath)
{
    BOOL                   bMore = FALSE, bFound = FALSE;
    HANDLE                 hSnapshot, hProcess, hThread;
    MODULEENTRY32W         me = {sizeof(me)};
    LPTHREAD_START_ROUTINE pThreadProc;

    // Create a snapshot of all modules in the specified process.
    if (INVALID_HANDLE_VALUE == (hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwPID)))
        return FALSE;

    bMore = Module32FirstW(hSnapshot, &me);
    for (; bMore; bMore = Module32NextW(hSnapshot, &me))
    {
        if (!_wcsicmp(me.szModule, szDllPath) || !_wcsicmp(me.szExePath, szDllPath))
        {
            bFound = TRUE;
            break;
        }
    }

    // If the DLL is found, proceed to eject it.
    if (!bFound)
    {
        CloseHandle(hSnapshot);
        return FALSE;
    }

    if (!(hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID)))
    {
        printf("OpenProcess(%lu) failed!!!\n", dwPID);
        CloseHandle(hSnapshot);
        return FALSE;
    }

    pThreadProc = (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "FreeLibrary");
    hThread     = CreateRemoteThread(hProcess, NULL, 0, pThreadProc, me.modBaseAddr, 0, NULL);
    WaitForSingleObject(hThread, INFINITE);

    CloseHandle(hThread);
    CloseHandle(hProcess);
    CloseHandle(hSnapshot);

    return TRUE;
}

int consumer_run();

struct ProcessInfo
{
    DWORD pid;
    wchar_t  name[ 1024 ];
};

DWORD WINAPI injector(LPVOID lpParam)
{
    struct ProcessInfo *p = (ProcessInfo *)lpParam;

    // std::cout << "Injecting dll: " << p->name << " into process with pid: " << p->pid << endl;
    if (!InjectDll(p->pid, p->name)) {
        std::cerr << "Injection failed!" << std::endl;
    }
    delete p;
    return 0;
}


int main( int argc, char *argv[] ) {
    DWORD pid = GetPIDbyProcName( L"Leak.X64.exe" );
    if ( pid == 0 ) {
        return -1;
    }

    LPVOID pParam = new ProcessInfo{ pid, L"E:\\github\\detours-cmake\\_build64\\bin\\RELEASE\\dllheap.dll" };
    DWORD creationStatus = 0;
    HANDLE hThread = ::CreateThread( NULL, 0, injector, pParam, 0, &creationStatus );

    Sleep(3000);
    consumer_run();

    DWORD waitStatus = WaitForSingleObject(hThread, INFINITE);

    return 0;
}
