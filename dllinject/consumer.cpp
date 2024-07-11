#include <windows.h>
#include <iostream>

const char* sharedMemoryName = "Local\\MySharedMemory";
const char* eventNameProduce = "Local\\ProduceEvent";
const char* eventNameConsume = "Local\\ConsumeEvent";
const int bufferSize = 256;

int consumer_run() {
    // Open the named event for producing data
    HANDLE hProduceEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, eventNameProduce);
    if (hProduceEvent == NULL) {
        std::cerr << "OpenEvent (Produce) failed: " << GetLastError() << std::endl;
        return 1;
    }

    // Open the named event for consuming data
    HANDLE hConsumeEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, eventNameConsume);
    if (hConsumeEvent == NULL) {
        std::cerr << "OpenEvent (Consume) failed: " << GetLastError() << std::endl;
        return 1;
    }

    // Open the memory-mapped file
    HANDLE hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, sharedMemoryName);
    if (hMapFile == NULL) {
        std::cerr << "OpenFileMapping failed: " << GetLastError() << std::endl;
        return 1;
    }

    // Map the memory-mapped file into the address space of the current process
    char* pBuf = (char*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, bufferSize);
    if (pBuf == NULL) {
        std::cerr << "MapViewOfFile failed: " << GetLastError() << std::endl;
        CloseHandle(hMapFile);
        return 1;
    }

    while (true) {
        // Wait for the producer to produce data
        WaitForSingleObject(hProduceEvent, INFINITE);

        // Read the data from the shared memory
        std::cout << "Consumed data: " << pBuf << std::endl;

        // Signal the producer that data is consumed
        SetEvent(hConsumeEvent);
    }

    // Clean up
    UnmapViewOfFile(pBuf);
    CloseHandle(hMapFile);
    CloseHandle(hProduceEvent);
    CloseHandle(hConsumeEvent);

    return 0;
}
