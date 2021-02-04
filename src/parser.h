#ifndef PARSER_H
#define PARSER_H

#include "types.h"

// NOTE: That was a lie. This isn't logcat format, this is format
// that Andoid Studio uses to format logcat logs: 
// date time PID-TID/package priority/tag: message
// 
// actual logcat format looks like this:
// date time PID TID Priority Tag: Message
//
// which is harder to parse but will be good enough

// NOTE: logcat can show move formats using -v specifier, but they doesn't give
// more information

enum Token_Type {
    Token_Unknown,
    
    Token_Colon,
    
    // NOTE: Those two was used to parse Android Studio format, 
    // can be used in future
    // Token_Dash,
    // Token_Slash,
    
    Token_Number,
    Token_String,
    
    Token_Date,
    Token_Time,
    
    Token_Equal,
    
    Token_SingleCharacter,
    
    Token_EndOfStream
};

struct Token
{
    Token_Type type;
    
    char* text;
    i32 length;
};

struct Tokenizer {
    char* position;
    bool parsing;
};

struct LogData;

// TODO: change pointer to some kind of bucket array
LogData* ParseMessage(char* message, int* messagesCount);



#endif // PARSER_H