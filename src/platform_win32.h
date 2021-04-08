#ifndef PLATFORM_WIN32_H
#define PLATFORM_WIN32_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

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

#endif //PLATFORM_WIN32_H
