#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include <windows.h>

char* messages[] = {
    "01-18 13:40:04.232  1817  1817 I SystemServiceManager: Starting com.android.server.power.ThermalManagerService",
    "01-18 13:40:04.235  1817  1817 I SystemServer: InitPowerManagement",
    "01-18 13:40:04.237  1817  1817 I SystemServer: StartRecoverySystemService",
    "01-18 13:40:04.237  1817  1817 I SystemServiceManager: Starting com.android.server.RecoverySystemService",
    "01-18 13:40:04.240  1817  1817 W RescueParty: Noticed 1 events for UID 0 in last 8 sec",
    "01-18 13:40:04.240  1817  1817 I SystemServer: StartLightsService",
    "01-18 13:40:04.240  1817  1817 I SystemServiceManager: Starting com.android.server.lights.MiuiLightsService",
};

int main(int argc, char* argv[]) {
    srand(time(NULL));
    
    int messagesCount = (int) sizeof(messages) / sizeof(*messages);
    
    while(true) {
        int index = rand() % messagesCount;
        
        DWORD written;
        char* msg = messages[index];
        WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), msg, strlen(msg), &written, NULL);
        
        Sleep(1000);
    }
}