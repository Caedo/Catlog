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
    char tag[64];
    LogPriority priority;
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


template<typename T>
struct CL_Array {
    i32 count;
    i32 capacity;
    T* data;
    
    inline T& operator[](int i) { assert(i >= 0 && i < count); return data[i]; }
    
    inline void Resize(i32 newSize) {
        if(capacity >= newSize)
            return;
        
        T* newData = (T*) malloc(newSize * sizeof(T));
        assert(newData);
        
        if(data) {
            memcpy(newData, data, sizeof(T) * count);
            free(data);
        }
        
        capacity = newSize;
        data = newData;
    }
    
    inline void Add(const T& n) {
        if(count >= capacity) {
            Resize(count + 1);
        }
        
        memcpy(&data[count], &n, sizeof(n));
        count++;
    }
    
    inline void AddEmpty() {
        if(count >= capacity) {
            Resize(count + 1);
        }
        
        data[count] = {};
        count++;
    }
    
    inline void RemoveAt(i32 index) {
        assert(index < count);
        
        if(index == count - 1) {
            count--;
            return;
        }
        
        int delta = count - index - 1;
        memmove(&data[index], &data[index + 1], sizeof(T) * delta);
        
        count--;
    }
};


#endif //TYPES_H
