#ifndef PLATFORM_WIN32_H
#define PLATFORM_WIN32_H

#include <windows.h>

#include "types.h"

#define BUFFER_SIZE 4096

struct ProcessData {
    PROCESS_INFORMATION pinfo;
    
    bool isRunning;

    int avaibleHandlesCount;
    HANDLE handles[3];
    
    OVERLAPPED outO = {};
    OVERLAPPED errO = {};
    
    HANDLE outEvent;
    HANDLE errEvent;
    
    HANDLE childOutRead;
    HANDLE childErrRead;
};

ProcessData SpawnProcess(char* path);
char* ReadProcessOut(ProcessData* pData);
int CloseProcess(ProcessData* pData);

#endif //PLATFORM_WIN32_H
