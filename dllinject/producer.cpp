#include "producer.h"
#include <iostream>

using namespace std;

const char* sharedMemoryName = "Local\\MySharedMemory";
const char* eventNameProduce = "Local\\ProduceEvent";
const char* eventNameConsume = "Local\\ConsumeEvent";
const int bufferSize = 256;

Producer::Producer()
{
    Init();
}

Producer::~Producer()
{
    if (m_pBuf != nullptr) {
        UnmapViewOfFile(m_pBuf);
    }
    if (m_hMapFile != nullptr) {
        CloseHandle(m_hMapFile);
    }

    CloseHandle(m_hConsumeEvent);
    CloseHandle(m_hProduceEvent);
}

void Producer::Send(const char* pData)
{
    if (m_pBuf == nullptr) {
        return;
    }

    // Wait for the consumer to consume previous data
    WaitForSingleObject(m_hConsumeEvent, INFINITE);

    // Copy the data to the shared memory
    std::strcpy(m_pBuf, pData);

    // Signal the consumer that data is produced
    SetEvent(m_hProduceEvent);
}

void Producer::Init()
{
    // Create a named event for producing data
    m_hProduceEvent = CreateEvent(NULL, FALSE, FALSE, eventNameProduce);
    if (m_hProduceEvent == NULL) {
        std::cerr << "CreateEvent (Produce) failed: " << GetLastError() << std::endl;
        return;
    }

    // Create a named event for consuming data
    m_hConsumeEvent = CreateEvent(NULL, FALSE, TRUE, eventNameConsume);
    if (m_hConsumeEvent == NULL) {
        std::cerr << "CreateEvent (Consume) failed: " << GetLastError() << std::endl;
        return;
    }

    // Create a memory-mapped file
    m_hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, bufferSize, sharedMemoryName);
    if (m_hMapFile == NULL) {
        std::cerr << "CreateFileMapping failed: " << GetLastError() << std::endl;
        return;
    }

    // Map the memory-mapped file into the address space of the current process
    m_pBuf = (char*)MapViewOfFile(m_hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, bufferSize);
    if (m_pBuf == NULL) {
        std::cerr << "MapViewOfFile failed: " << GetLastError() << std::endl;
        CloseHandle(m_hMapFile);
        return;
    }
}
