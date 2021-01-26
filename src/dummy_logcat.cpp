#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include <windows.h>

char* messages[] = {
    "01-18 13:40:04.232  1817  1817 I SystemServiceManager: Starting com.android.server.power.ThermalManagerService\n",
    "01-18 13:40:04.235  1817  1817 I SystemServer: InitPowerManagement\n",
    "01-18 13:40:04.237  1817  1817 I SystemServer: StartRecoverySystemService\n",
    "01-18 13:40:04.237  1817  1817 I SystemServiceManager: Starting com.android.server.RecoverySystemService\n",
    "01-18 13:40:04.240  1817  1817 W RescueParty: Noticed 1 events for UID 0 in last 8 sec\n",
    "01-18 13:40:04.240  1817  1817 I SystemServer: StartLightsService\n",
    "01-18 13:40:04.240  1817  1817 I SystemServiceManager: Starting com.android.server.lights.MiuiLightsService\n",
};

int main(int argc, char* argv[]) {
    srand(time(NULL));
    
    int messagesCount = (int) sizeof(messages) / sizeof(*messages);
    
    while(true) {
        int index = rand() % messagesCount;
        char* msg = messages[index];
        
        printf(msg);
        fflush(stdout);
        
        Sleep(1000);
    }
}