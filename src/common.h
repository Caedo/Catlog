#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

typedef uint8_t u8;
typedef int8_t  s8;

typedef uint16_t u16;
typedef int16_t i16;

typedef uint32_t u32;
typedef int32_t i32;

typedef uint64_t u64;
typedef int64_t i64;

struct Date {
    i32 day;
    i32 month;
    i32 year;
};

struct Time {
    i32 hours;
    i32 minutes;
    float seconds;
};

enum LogPriority {
    None, // debug value
    Verbose,
    Debug,
    Info,
    Warning,
    Error,
    Fatal,
    Silent
};

char* LogPriorityName[] = {
    "None",
    "Verbose",
    "Debug",
    "Info",
    "Warning",
    "Error",
    "Fatal",
    "Silent"
};

struct LogData {
    Date date;
    Time time;
    i32 PID;
    i32 TID;
    
    LogPriority priority;
    
    char* rawString;
    char* tag;
    char* message;
    
    bool parseFailed;
    bool messageIncomplete;
};

#endif //TYPES_H
