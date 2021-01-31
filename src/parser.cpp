#include "parser.h"

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

bool IsTokenEqual(Token token, char* match) {
    char* at = match;
    for (int i = 0; i < token.length; i++)
    {
        if(*at == 0 || token.text[i] != *at) {
            return false;
        }
        
        at++;
    }
    
    return *at == 0;
}

char* GetStringFromToken(Token token) {
    char* ret = (char*) malloc(token.length + 1);
    
    StringCopy(ret, token.text, token.length);
    ret[token.length] = '\0';
    
    return ret;
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
        bool haveDashes = false;
        bool haveColons = false;
        
        while(IsWhiteSpace(*at) == false) {
            if(*at == '-')      haveDashes = true;
            else if(*at == ':') haveColons = true;
            
            at++;
        }
        
        if(haveColons && haveDashes) 
            token.type = Token_Unknown;
        else if(haveColons) token.type = Token_Time;
        else if(haveDashes) token.type = Token_Date;
        else                token.type = Token_Number;
        
        token.length = at - token.text;
    }
    else if(IsAlpha(firstChar)) {
        token.type = Token_String;
        
        while(IsWhiteSpace(*at) == false && *at != ':') {
            at++;
        }
        
        token.length = at - token.text;
        
        if(token.length == 1) {
            token.type = Token_SingleCharacter;
        }
    }
    else {
        switch(firstChar) {
            case ':': token.type = Token_Colon; break;
            case '=': token.type = Token_Equal; break;
            
            // NOTE: Will be probably used to parse Android studio format
            // case '-': token.type = Token_Dash;  break;
            // case '/': token.type = Token_Slash; break;
            
            case '\0': token.type = Token_EndOfStream; tokenizer->parsing = false; break;
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

Date ParseDate(Token token) {
    Date date = {};
    
    char buff[5];
    
    // check if 'year' is present
    char *at = token.text;
    while(IsNumber(*at)) {
        at++;
    }
    
    int firstPartLen = at - token.text;
    bool yearPresent = firstPartLen > 2;
    
    if(yearPresent) {
        StringCopy(buff, token.text, 4);
        buff[4] = 0;
        
        date.year = atoi(buff);
        token.text += 5;
    }
    
    StringCopy(buff, token.text, 2);
    buff[2] = 0;
    date.month = atoi(buff);
    token.text += 3;
    
    StringCopy(buff, token.text, 2);
    buff[2] = 0;
    date.day = atoi(buff);
    
    return date;
}

Time ParseTime(Token token) {
    Time time = {};
    
    char buff[7];
    
    StringCopy(buff, token.text, 2);
    buff[2] = 0;
    time.hours = atoi(buff);
    token.text += 3;
    
    StringCopy(buff, token.text, 2);
    buff[2] = 0;
    time.minutes = atoi(buff);
    token.text += 3;
    
    StringCopy(buff, token.text, 6);
    buff[6] = 0;
    time.seconds = atof(buff);
    
    return time;
}

LogPriority ParsePriority(Token token) {
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

void CopyTextUntilEndOfLine(Tokenizer* tokenizer, char* destination) {
    int length = 0;
    
    EatWhiteSpaces(tokenizer);
    
    char* at = tokenizer->position;
    while(*at != '\n' && *at != '\0') {
        length++;
        at++;
    }
    
    StringCopy(destination, tokenizer->position, length);
    destination[length] = 0;
    
    tokenizer->position += length;
}

ParserResult ParseMessage(char* message) {
    assert(message != NULL);
    
    ParserResult ret = {};
    
    Tokenizer tokenizer = {};
    tokenizer.position = message;
    tokenizer.parsing = true;
    
    int index = 0;
    
    while(tokenizer.parsing && index < MAX_PARSE_COUNT) {
        LogData logData = {};
        
        bool reachedEndOfLine = false;
        char* lineStart = tokenizer.position;
        
        Token token = GetNextToken(&tokenizer);
        while(reachedEndOfLine == false) {
            switch(token.type) {
                case Token_Date: {
                    logData.date = ParseDate(token);
                }
                break;
                
                case Token_Time: {
                    logData.time = ParseTime(token);
                }
                break;
                
                case Token_Number: {
                    if(logData.PID == 0) {
                        logData.PID = GetFloat(token);
                    }
                    else {
                        logData.TID = GetFloat(token);
                    }
                }
                break;
                
                case Token_SingleCharacter: {
                    logData.priority = ParsePriority(token);
                }
                break;
                
                case Token_String: {
                    logData.tag = GetStringFromToken(token);
                }
                break;
                
                case Token_Colon: {
                    logData.message = GetTextUntilEndOfLine(&tokenizer);
                    reachedEndOfLine = true;
                }
                break;
                
                case Token_Unknown: {
                    tokenizer.position = lineStart;
                    logData.message = GetTextUntilEndOfLine(&tokenizer);
                    logData.parseFailed = true;
                    
                    reachedEndOfLine = true;
                }
                break;
                
                case Token_EndOfStream: {
                    tokenizer.parsing = false;
                    reachedEndOfLine = true;
                }
                break;
            }
            
            if(reachedEndOfLine == false)
                token = GetNextToken(&tokenizer);
        }
        
        if(tokenizer.parsing)
            ret.data[index++] = logData;
    }
    
    ret.messagesCount = index;
    return ret;
}

void ParseSettingsFile(Settings* settings, char* fileContent) {
    assert(fileContent != NULL);
    
    ParserResult ret = {};
    
    Tokenizer tokenizer = {};
    tokenizer.position = fileContent;
    tokenizer.parsing = true;
    
    while(tokenizer.parsing) {
        Token key = RequireToken(&tokenizer, Token_String);
        RequireToken(&tokenizer, Token_Equal);
        
        if(tokenizer.parsing == false) break;
        
        if(IsTokenEqual(key, "path_to_adb")) {
            CopyTextUntilEndOfLine(&tokenizer, settings->pathToAdb);
        }
        else {
            tokenizer.parsing = false;
        }
    }
}