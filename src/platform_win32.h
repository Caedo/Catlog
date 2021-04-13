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

//Takes path to ADB instance and CLArray containing ADB arguments
//Then invokes SpawnProcess(char* path) with startup command built of said arguments
ProcessData SpawnProcess(char* path, CLArray<char*> params);

//Takes path to ADB instance, number of desired ADB arguments, and said arguments
//Then invokes SpawnProcess(char* path) with startup command built of said arguments
ProcessData SpawnProcess(int param_count, char* path, ...);

//Takes path combined with all startup arguments. Path is actually full startup command
ProcessData SpawnProcess(char* path);

#endif //PLATFORM_WIN32_H
