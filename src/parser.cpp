#include "parser.h"

#define MAX_MESSAGES 200

inline bool IsWhiteSpace(char c) {
    return c == ' '  ||
        c == '\n' ||
        c == '\t' ||
        c == '\r';
}

inline bool IsEndOfLine(char c) {
    return c == '\n' || c == '\r';
}

inline bool IsAlpha(char c) {
    return (c >= 'a' && c <= 'z') || 
        (c >= 'A' && c <= 'Z');
}

inline bool IsNumber(char c) {
    return (c >= '0' && c <= '9');
}

inline void Move(Tokenizer* tokenizer, Token token) {
    tokenizer->position = tokenizer->position + token.length;
}

void StringCopy(char* dest, char* source, int length) {
    for(int i = 0; i < length; i++) {
        dest[i] = source[i];
    }
}

// TODO: Add GetNumber function
float GetFloat(Token token) {
    if(token.length == 0) 
        return 0;
    
    // TODO: Get rid off this temporary allocation...
    char* temp = (char*) malloc(token.length + 1);
    temp[token.length] = 0;
    
    StringCopy(temp, token.text, token.length);
    
    float result = atof(temp);
    free(temp);
    
    return result;
}


void EatWhiteSpaces(Tokenizer* tokenizer) {
    while(IsWhiteSpace(*tokenizer->position))
        tokenizer->position++;
}

Token PeekNextToken(Tokenizer* tokenizer) {
    Token token = {};
    
    if(tokenizer->parsing == false) {
        return token;
    }
    
    EatWhiteSpaces(tokenizer);
    
    char* at = tokenizer->position;
    char firstChar = *(tokenizer->position);
    
    token.text = at;
    
    if(IsNumber(firstChar)) {
        token.type = Token_Number;
        
        while(IsNumber(*at) || *at == '.') {
            at++;
        }
        
        token.length = at - token.text;
    }
    // @HACK: 'package' can sometimes be "?" 
    else if(IsAlpha(firstChar) || *at == '?') {
        token.type = Token_String;
        
        while(IsAlpha(*at) || *at == '.' || *at == '?') {
            at++;
        }
        
        token.length = at - token.text;
    }
    else {
        switch(firstChar) {
            case '-': token.type = Token_Dash;  break;
            case ':': token.type = Token_Colon; break;
            case '/': token.type = Token_Slash; break;
            case '\0': token.type = Token_EndOfStream; break;
        }
        
        token.length = 1;
    }
    
    return token;
}

Token GetNextToken(Tokenizer* tokenizer) {
    Token token = PeekNextToken(tokenizer);
    Move(tokenizer, token);
    
    return token;
}

Token RequireToken(Tokenizer* tokenizer, int expectedType)
{
    Token token = GetNextToken(tokenizer);
    if(token.type != expectedType) {
        tokenizer->parsing = false;
    }
    
    return token;
}

Date ParseDate(Tokenizer* tokenizer) {
    Date date = {};
    
    Token dayToken = RequireToken(tokenizer, Token_Number);
    RequireToken(tokenizer, Token_Dash);
    Token monthToken = RequireToken(tokenizer, Token_Number);
    
    Token optionalDashToken = PeekNextToken(tokenizer);
    Token yearToken = {};
    if(optionalDashToken.type == Token_Dash) {
        Move(tokenizer, optionalDashToken);
        yearToken = RequireToken(tokenizer, Token_Number);
    }
    
    if(tokenizer->parsing) {
        date.day   = GetFloat(dayToken);
        date.month = GetFloat(monthToken);
        date.year  = GetFloat(yearToken);
    }
    
    return date;
}

Time ParseTime(Tokenizer* tokenizer) {
    Time time = {};
    
    Token hoursToken = RequireToken(tokenizer, Token_Number);
    RequireToken(tokenizer, Token_Colon);
    Token minutesToken = RequireToken(tokenizer, Token_Number);
    RequireToken(tokenizer, Token_Colon);
    Token secondsToken = RequireToken(tokenizer, Token_Number);
    
    if(tokenizer->parsing) {
        time.hours   = GetFloat(hoursToken);
        time.minutes = GetFloat(minutesToken);
        time.seconds = GetFloat(secondsToken);
    }
    
    return time;
}

LogPriority ParsePriority(Tokenizer* tokenizer) {
    Token token = RequireToken(tokenizer, Token_String);
    if(token.length != 1) {
        tokenizer->parsing = false;
        return None;
    }
    
    switch(*token.text) {
        case 'V': return Verbose;
        case 'D': return Debug;
        case 'I': return Info;
        case 'W': return Warning;
        case 'E': return Error;
        case 'A': return Assert;
        case 'F': return Assert;
        case 'S': return Assert;
        default:  return None;
    }
}

void PeekAndMoveIfCorrectType(Tokenizer* tokenizer, int type) {
    Token token = PeekNextToken(tokenizer);
    if(token.type == type) {
        Move(tokenizer, token);
    }
}

char* GetTextUntilEndOfLine(Tokenizer* tokenizer) {
    int length = 0;
    
    EatWhiteSpaces(tokenizer);
    
    char* at = tokenizer->position;
    while(*at != '\n' && *at != '\0') {
        length++;
        at++;
    }
    
    char* str = (char*) malloc((length + 1) * sizeof(char));
    StringCopy(str, tokenizer->position, length);
    str[length] = 0;
    
    tokenizer->position += length;
    
    return str;
}

LogData* ParseMessage(char* message, int* messagesCount) {
    LogData* ret = (LogData*) malloc(MAX_MESSAGES * sizeof(LogData));
    
    Tokenizer tokenizer = {};
    tokenizer.position = message;
    tokenizer.parsing = true;
    
    int index = 0;
    
    while(tokenizer.parsing && index < MAX_MESSAGES) {
        LogData logData = {};
        
        Token testToken = PeekNextToken(&tokenizer);
        if(testToken.type == Token_EndOfStream) {
            break;
        }
        
        if(testToken.type != Token_Number) {
            logData.message = GetTextUntilEndOfLine(&tokenizer);
            ret[index++] = logData;
            
            continue;
        }
        
        logData.date = ParseDate(&tokenizer);
        logData.time = ParseTime(&tokenizer);
        
        Token token = RequireToken(&tokenizer, Token_Number);
        logData.PID = GetFloat(token);
        
        PeekAndMoveIfCorrectType(&tokenizer, Token_Dash);
        
        token = RequireToken(&tokenizer, Token_Number);
        logData.TID = GetFloat(token);
        
        PeekAndMoveIfCorrectType(&tokenizer, Token_Slash);
        
        /*
        token = RequireToken(&tokenizer, Token_String);
        logData.package = (char*) malloc((token.length + 1) * sizeof(char));
        StringCopy(logData.package, token.text, token.length);
        logData.package[token.length] = 0;
        */
        
        logData.priority = ParsePriority(&tokenizer);
        
        PeekAndMoveIfCorrectType(&tokenizer, Token_Slash);
        
        token = RequireToken(&tokenizer, Token_String);
        logData.tag = (char*) malloc((token.length + 1) * sizeof(char));
        StringCopy(logData.tag, token.text, token.length);
        logData.tag[token.length] = 0;
        
        RequireToken(&tokenizer, Token_Colon);
        logData.message = GetTextUntilEndOfLine(&tokenizer);
        
        if(tokenizer.parsing == false)
            break;
        
        ret[index++] = logData;
    }
    
    *messagesCount = index;
    return ret;
}