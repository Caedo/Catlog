#include "platform_win32.h"
#include "types.h"

static void CreateNamepPipePair(HANDLE* read, HANDLE* write, DWORD bufferSize, SECURITY_ATTRIBUTES* attributes)
{
    static int id = 0;
    char name[MAX_PATH];
    wsprintfA(name, "\\\\.\\pipe\\Catlog.%08x.%08x", GetCurrentProcessId(), id++);
    
    *read = CreateNamedPipeA(
                             name,
                             PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
                             PIPE_TYPE_BYTE | PIPE_WAIT,
                             1,
                             bufferSize,
                             bufferSize,
                             0,
                             attributes
                             );
    assert(*read != INVALID_HANDLE_VALUE);
    
    *write = CreateFileA(
                         name,
                         GENERIC_WRITE,
                         0,
                         attributes,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                         NULL
                         );
    
    assert(*write != INVALID_HANDLE_VALUE);
}


ProcessData SpawnProcess(char* path) {
    BOOL ok;
    
    ProcessData pData = {};
    
    SECURITY_ATTRIBUTES sAttribs = {};
    sAttribs.nLength = sizeof(sAttribs);
    sAttribs.bInheritHandle = TRUE;
    
    HANDLE childOutWrite;
    CreateNamepPipePair(&pData.childOutRead, &childOutWrite, BUFFER_SIZE, &sAttribs);
    
    HANDLE childErrWrite;
    CreateNamepPipePair(&pData.childErrRead, &childErrWrite, BUFFER_SIZE, &sAttribs);
    
    STARTUPINFOA sinfo = {};
    sinfo.cb = sizeof(sinfo);
    sinfo.dwFlags = STARTF_USESTDHANDLES;
    sinfo.hStdOutput = childOutWrite;
    sinfo.hStdError = childErrWrite;
    
    
    ok = CreateProcess(
                        NULL,
                        TEXT(path),
                        NULL,
                        NULL,
                        TRUE,
                        CREATE_NO_WINDOW | CREATE_SUSPENDED,
                        NULL,
                        NULL,
                        &sinfo,
                        &pData.pinfo
                        );
    
    assert(ok);
    
    DWORD resumeResult = ResumeThread(pData.pinfo.hThread);
    assert(resumeResult);
    
    CloseHandle(pData.pinfo.hThread);
    CloseHandle(childOutWrite);
    CloseHandle(childErrWrite);
    
    pData.outEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
    pData.errEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
    
    pData.handles[0] = pData.outEvent;
    pData.handles[1] = pData.errEvent;
    pData.handles[2] = pData.pinfo.hProcess;
    
    pData.avaibleHandlesCount = 3;
    
    return pData;
}

bool ReadProcessOut(ProcessData* pData, char* buffer) {
    static char errorBuffer[BUFFER_SIZE];
    
    if(pData->avaibleHandlesCount == 0) 
        return false;
    
    BOOL ok;
    
    DWORD wait = WaitForMultipleObjects(pData->avaibleHandlesCount, pData->handles, FALSE, 0);
    if((wait >= WAIT_OBJECT_0 && wait < WAIT_OBJECT_0 + pData->avaibleHandlesCount) == false) {
        if(wait == (DWORD)0xFFFFFFFF) {
            fprintf(stderr, "Error: %d", GetLastError());
        }
        
        return false;
    }
    
    DWORD index = wait - WAIT_OBJECT_0;
    HANDLE h = pData->handles[index];
    if (h == pData->outEvent)
    {
        if (pData->outO.hEvent != NULL)
        {
            DWORD r;
            if (GetOverlappedResult(pData->childOutRead, &pData->outO, &r, TRUE))
            {
                printf("STDOUT received: %.*s\n", (int)r, buffer);
                memset(&pData->outO, 0, sizeof(pData->outO));
                
                return true;
            }
            else
            {
                assert(GetLastError() == ERROR_BROKEN_PIPE);
                
                pData->handles[index] = pData->handles[pData->avaibleHandlesCount - 1];
                pData->avaibleHandlesCount--;
                
                CloseHandle(pData->childOutRead);
                CloseHandle(pData->outEvent);
                
                return false;
            }
        }
        
        memset(buffer, 0, 4096);
        
        pData->outO.hEvent = pData->outEvent;
        ReadFile(pData->childOutRead, buffer, BUFFER_SIZE, NULL, &pData->outO);
    }
    else if (h == pData->errEvent)
    {
        if (pData->errO.hEvent != NULL)
        {
            DWORD read;
            if (GetOverlappedResult(pData->childErrRead, &pData->errO, &read, TRUE))
            {
                fprintf(stderr, "STDERR received: %.*s\n", (int)read, errorBuffer);
                memset(&pData->errO, 0, sizeof(pData->errO));
            }
            else
            {
                assert(GetLastError() == ERROR_BROKEN_PIPE);
                
                pData->handles[index] = pData->handles[pData->avaibleHandlesCount - 1];
                pData->avaibleHandlesCount;
                
                CloseHandle(pData->childErrRead);
                CloseHandle(pData->errEvent);
                
                return false;
            }
        }
        
        pData->errO.hEvent = pData->errEvent;
        ReadFile(pData->childErrRead, errorBuffer, BUFFER_SIZE, NULL, &pData->errO);
    }
    else if (h == pData->pinfo.hProcess)
    {
        DWORD exitCode;
        ok = GetExitCodeProcess(pData->pinfo.hProcess, &exitCode);
        assert(ok);
        
        for(int i = 0; i < pData->avaibleHandlesCount; i++) {
            CloseHandle(pData->handles[i]);
        }
        pData->avaibleHandlesCount = 0;
        
        
        fprintf(stderr, "exit code = %u\n", exitCode);
    }
    
    
    return false;
}

int CloseProcess(ProcessData* data) {
    
    TerminateProcess(data->pinfo.hProcess, 0);
    CloseHandle(data->pinfo.hProcess);
    
    return 0;
}

bool OpenFileDialog(char* pathBuffer, s32 maxPath) {
    OPENFILENAME ofn = {};       // common dialog box structure
    
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL; // TODO: Add GLFW window handle
    ofn.lpstrFile = pathBuffer;
    // Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
    // use the contents of szFile to initialize itself.
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = maxPath;
    ofn.lpstrFilter = "All\0*.*\0Text\0*.TXT\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    
    // Display the Open dialog box. 
    
    return GetOpenFileName(&ofn) == TRUE;
}