#ifndef PARSER_H
#define PARSER_H

// NOTE: logcat format:
// date time PID-TID/package priority/tag: message

struct LogData;

enum TokenType {
    Token_Unknown,
    
    Token_Dash,
    Token_Colon,
    Token_Slash,
    
    Token_Number,
    Token_String,
    
    Token_EndOfStream
};

struct Token
{
    TokenType type;
    
    char* text;
    int length;
};

struct Tokenizer {
    char* position;
    bool parsing;
};

// TODO: change pointer to some kind of bucket array
LogData* ParseMessage(char* message, int* messagesCount);



#endif // PARSER_H