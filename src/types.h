#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

typedef uint8_t u8;
typedef int8_t  s8;

typedef uint16_t u16;
typedef int16_t s16;

typedef uint32_t u32;
typedef int32_t s32;

typedef uint64_t u64;
typedef int64_t s64;

struct Date {
    int year;
    int month;
    int day;
};

struct Time {
    int hours;
    int minutes;
    float seconds;
};

enum LogPriority {
    None, // debug value
    Verbose,
    Debug,
    Info,
    Warning,
    Error,
    Assert,
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
    "Assert",
    "Fatal",
    "Silent"
};

struct LogData {
    Date date;
    Time time;
    int PID;
    int TID;
    
    LogPriority priority;
    
    char* tag;
    char* message;
    
    bool parseFailed;
};

struct TagPriorityPair {
    char priority;
    char tag[256];
};

#define MAX_PARSE_COUNT 20
struct ParserResult {
    LogData data[MAX_PARSE_COUNT];
    int messagesCount;
};

struct Settings {
    bool isDirty;
    char pathToAdb[1024]; // TODO: dynamic string?
};

#endif //TYPES_H
