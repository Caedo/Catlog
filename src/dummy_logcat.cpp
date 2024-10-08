#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include <windows.h>

#include "common.h"

//char* messages[] = {
//"01-18 13:40:04.232  1817  1817 I SystemServiceManager: Starting com.android.server.power.ThermalManagerService\n",
//"01-18 13:40:04.235  1817  1817 I SystemServer: InitPowerManagement\n",
//"01-18 13:40:04.237  1817  1817 I SystemServer: StartRecoverySystemService\n",
//"01-18 13:40:04.237  1817  1817 I SystemServiceManager: Starting com.android.server.RecoverySystemService\n",
//"01-18 13:40:04.240  1817  1817 W RescueParty: Noticed 1 events for UID 0 in last 8 sec\n",
//"01-18 13:40:04.240  1817  1817 I SystemServer: StartLightsService\n",
//"01-18 13:40:04.240  1817  1817 I SystemServiceManager: Starting com.android.server.lights.MiuiLightsService\n",
//};

char PriorityToChar(LogPriority priority) {
    switch(priority) {
        case None: return 'N';
        case Verbose: return 'V';
        case Debug: return 'D';
        case Info: return 'I';
        case Warning: return 'W';
        case Error: return 'E';
        case Fatal: return 'F';
        case Silent: return 'S';
    }
    
    return '?';
}


struct TestMessage {
    Date date;
    Time time;
    int PID;
    int TID;
    
    LogPriority priority;
    
    char* tag;
    char* message;
};


TestMessage messages[] = {
    {{1, 18, 2021}, {13, 40, 4.232}, 1817, 1817, Info, "SystemServiceManager", "Starting %f com.android.server.power.ThermalManagerService"},
    {{1, 18, 2021}, {13, 40, 4.232}, 1817, 1817, Warning, "SystemServer", "Test %d warning message"},
    {{1, 18, 2021}, {13, 40, 4.232}, 1817, 1817, Error, "SystemServer", "Test warning message"},
    {{1, 18, 2021}, {13, 40, 4.232}, 1817, 1817, Verbose, "SystemServer", "Test warning message"},
    {{1, 18, 2021}, {13, 40, 4.232}, 1817, 1817, Debug, "SystemServer", "Test warning message"},
};

char* devices[] = {"Test 1 :>\tdevice", "Test 2 :|\tdevice", "Test 3 :<\tdevice"};

int main(int argc, char* argv[]) {
    const int messagesPerBatch = 3;
    srand(time(NULL));
    
    if(argc == 1) {
        return 0;
    }
    
    if(strcmp(argv[1], "logcat") == 0) {
        int messagesCount = (int) sizeof(messages) / sizeof(*messages);
        while(true) {
            for(int i = 0; i < messagesPerBatch; i++) {
                int index = rand() % messagesCount;
                TestMessage log = messages[index];
                
                printf("%d-%d %d:%d:%.3f %d %d %c %s: %s\n", log.date.month, log.date.day, log.time.hours, log.time.minutes, log.time.seconds, log.PID, log.TID, PriorityToChar(log.priority), log.tag, log.message);
            }
            
            fflush(stdout);
            Sleep(50);
        }
    }
    
    if(strcmp(argv[1], "devices") == 0) {
        Sleep(1000);
        
        printf("List of devices attached\n");
        
        Sleep(rand() % 500 + 250);
        
        for(int i = 0; i < 3; i++) {
            printf(devices[i]);
            printf("\n");
        }
        
        fflush(stdout);
    }
}