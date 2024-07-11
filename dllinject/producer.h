#pragma once
#include <windows.h>

class Producer
{
public:
    Producer();
    ~Producer();
    void Send(const char* pData);
private:
    void Init();

private:
    HANDLE m_hProduceEvent = nullptr;
    HANDLE m_hConsumeEvent = nullptr;
    HANDLE m_hMapFile = nullptr;
    char* m_pBuf = nullptr;
};
