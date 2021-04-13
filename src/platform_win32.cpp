#include <stdarg.h>
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

ProcessData SpawnProcess(char* path, CLArray<char*> params){
    int launch_size = (int)strlen(path)+1;
    char* launch = (char*)malloc(launch_size);
    strcpy(launch, path);
    for(int i=0; i<params.count; i++){
        char* param = params[i];
        launch_size += (int)strlen(param)+1;
        launch = (char*)realloc(launch, launch_size);
        strcat(launch, " ");
        strcat(launch, param);
    }
    //TODO: Remove debugging console print
    //printf("%s\n", launch);
    
    return SpawnProcess(launch);
}

ProcessData SpawnProcess(int param_count, char* path, ...) {
    int launch_size = (int)strlen(path)+1;
    char* launch = (char*)malloc(launch_size);
    strcpy(launch, path);
    va_list params;
    va_start(params, path);
    for(int i=0; i<param_count; i++){
        char* param = va_arg(params, char*);
        launch_size += (int)strlen(param)+1;
        launch = (char*)realloc(launch, launch_size);
        strcat(launch, " ");
        strcat(launch, param);
    }
    //TODO: Remove debugging console print
    //printf("%s\n", launch);
    
    return SpawnProcess(launch);
}

ProcessData SpawnProcess(char* path) {
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
    
    BOOL createResult = CreateProcess(
                                      NULL,
                                      TEXT(path),
                                      NULL,
                                      NULL,
                                      TRUE,
                                      CREATE_NO_WINDOW,
                                      NULL,
                                      NULL,
                                      &sinfo,
                                      &pData.pinfo
                                      );
    
    if(!createResult) {
        return {};
    }
    
    CloseHandle(pData.pinfo.hThread);
    CloseHandle(childOutWrite);
    CloseHandle(childErrWrite);
    
    HANDLE outEventHandle = CreateEvent(NULL, TRUE, TRUE, NULL);
    HANDLE errorEventHandle = CreateEvent(NULL, TRUE, TRUE, NULL);
    
    if(!outEventHandle || !errorEventHandle) {
        TerminateProcess(pData.pinfo.hProcess, 0);
        CloseHandle(pData.pinfo.hProcess);
        
        if(outEventHandle)   CloseHandle(outEventHandle);
        if(errorEventHandle) CloseHandle(errorEventHandle);
        
        return {};
    }
    
    pData.outEvent = outEventHandle;
    pData.errEvent = errorEventHandle;
    
    pData.handles[0] = pData.outEvent;
    pData.handles[1] = pData.errEvent;
    pData.handles[2] = pData.pinfo.hProcess;
    
    pData.avaibleHandlesCount = 3;
    pData.isRunning = true;
    
    return pData;
}

int ReadProcessOut(ProcessData* pData, char* buffer, int bufferSize) {
    static char errorBuffer[BUFFER_SIZE];
    
    if(pData->avaibleHandlesCount == 0) 
        return 0;
    
    BOOL ok;
    
    MTR_BEGIN("main", "wait for objects");
    DWORD wait = WaitForMultipleObjects(pData->avaibleHandlesCount, pData->handles, FALSE, 0);
    MTR_END("main", "wait for objects");
    
    if((wait >= WAIT_OBJECT_0 && wait < WAIT_OBJECT_0 + pData->avaibleHandlesCount) == false) {
        if(wait == (DWORD)0xFFFFFFFF) {
            fprintf(stderr, "Error: %d", GetLastError());
        }
        
        return 0;
    }
    
    DWORD index = wait - WAIT_OBJECT_0;
    HANDLE h = pData->handles[index];
    if (h == pData->outEvent)
    {
        if (pData->outO.hEvent != NULL)
        {
            MTR_SCOPE("main", "Overlapped result");
            DWORD readBytes;
            if (GetOverlappedResult(pData->childOutRead, &pData->outO, &readBytes, TRUE))
            {
                //printf("STDOUT received: %.*s\n", (int)r, buffer);
                buffer[readBytes] = 0;
                memset(&pData->outO, 0, sizeof(pData->outO));
                
                return readBytes + 1;
            }
            else
            {
                assert(GetLastError() == ERROR_BROKEN_PIPE);
                
                pData->handles[index] = pData->handles[pData->avaibleHandlesCount - 1];
                pData->avaibleHandlesCount--;
                
                CloseHandle(pData->childOutRead);
                CloseHandle(pData->outEvent);
                
                return 0;
            }
        }
        
        MTR_BEGIN("main", "Read File");
        // we read size - 1 because we want to null terminate given string
        pData->outO.hEvent = pData->outEvent;
        ReadFile(pData->childOutRead, buffer, bufferSize - 1, NULL, &pData->outO);
        MTR_END("main", "Read File");
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
                
                return 0;
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
        
        pData->isRunning = false;
        
        fprintf(stderr, "exit code = %u\n", exitCode);
    }
    
    return 0;
}

int CloseProcess(ProcessData* data) {
    
    TerminateProcess(data->pinfo.hProcess, 0);
    CloseHandle(data->pinfo.hProcess);
    
    CloseHandle(data->outEvent);
    CloseHandle(data->errEvent);
    CloseHandle(data->childOutRead);
    CloseHandle(data->childErrRead);
    
    memset(data, 0, sizeof(ProcessData));
    
    return 0;
}

bool OpenFileDialog(CLStr* path, CLStr* fileName = nullptr) {
    assert(path);
    
    OPENFILENAME ofn = {};       // common dialog box structure
    
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL; // TODO: Add GLFW window handle
    ofn.lpstrFile = (LPSTR) path->str;
    
    // Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
    // use the contents of szFile to initialize itself.
    ofn.lpstrFile[0] = '\0';
    
    // NOTE(Coedo) WinApi uses 32 bit value for max path size
    ofn.nMaxFile = (DWORD) path->capacity; 
    ofn.lpstrFilter = "All\0*.*\0";
    ofn.nFilterIndex = 1;
    
    if(fileName != nullptr) {
        ofn.lpstrFileTitle = (LPSTR) fileName->str;
        ofn.nMaxFileTitle = (DWORD) fileName->capacity;
    }
    
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    
    // Display the Open dialog box. 
    BOOL result = GetOpenFileName(&ofn);
    if(result) {
        path->length = (u64) strlen((char*) path->str);
        
        if(fileName) {
            fileName->length = (u64) strlen((char*) fileName->str);
        }
    }
    
    return result;
}

bool OpenFileDialog(char* path, u32 pathBufferSize) {
    assert(path);
    
    CLStr pathStr = StrMakeBuffer(path, pathBufferSize);
    return OpenFileDialog(&pathStr);
}


bool SaveFileDialog(CLStr* path, CLStr* fileName = nullptr) {
    assert(path);
    
    OPENFILENAME ofn = {};       // common dialog box structure
    
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL; // TODO: Add GLFW window handle
    ofn.lpstrFile = (LPSTR) path->str;
    
    // Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
    // use the contents of szFile to initialize itself.
    ofn.lpstrFile[0] = '\0';
    
    // NOTE(Coedo) WinApi uses 32 bit value for max path size
    ofn.nMaxFile = (DWORD) path->capacity; 
    ofn.lpstrFilter = "All\0*.*\0";
    ofn.nFilterIndex = 1;
    
    if(fileName != nullptr) {
        ofn.lpstrFileTitle = (LPSTR) fileName->str;
        ofn.nMaxFileTitle = (DWORD) fileName->capacity;
    }
    
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT;
    
    // Display the Open dialog box. 
    BOOL result = GetSaveFileName(&ofn);
    if(result) {
        path->length = (u64) strlen((char*) path->str);
        
        if(fileName) {
            fileName->length = (u64) strlen((char*) fileName->str);
        }
    }
    
    return result;
}

bool SaveFileDialog(char* path, u32 pathBufferSize) {
    assert(path);
    
    CLStr pathStr = StrMakeBuffer(path, pathBufferSize);
    return SaveFileDialog(&pathStr);
}
