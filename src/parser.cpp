#include "parser.h"
#include "settings.h"
#include "parser_common.h"
#include "CLArray.h"


Token ADB_PeekNextToken(Tokenizer* tokenizer) {
    Token token = {};
    
    if(tokenizer->parsing == false) {
        return token;
    }
    
    EatWhiteSpaces(tokenizer);
    
    char* at = tokenizer->position;
    char firstChar = *(tokenizer->position);
    
    token.text = at;
    
    if(IsNumber(firstChar)) {
        
        while(IsNumber(*at) || *at == '.') {
            at++;
        }
        
        token.type = Token_Number;
        token.length = (i32) (at - token.text);
    }
    else if(IsAlpha(firstChar) || *at == '.') {
        token.type = Token_String;
        
        while(IsWhiteSpace(*at) == false && *at != ':') {
            at++;
        }
        
        token.length = (i32) (at - token.text);
    }
    else if(IsEndOfLine(*at)) {
        token.type = Token_EndOfLine;
        
        // Can be multiple characters
        while(IsEndOfLine(*at)) {
            at++;
        }
        
        token.length = (i32) (at - token.text);
    }
    else {
        switch(firstChar) {
            case ':': token.type = Token_Colon; break;
            case '-': token.type = Token_Dash;  break;
            case '=': token.type = Token_Equal; break;
            
            case '\0':{
                token.type = Token_EndOfStream;
                tokenizer->parsing = false;
            }
            break;
        }
        
        token.length = 1;
    }
    
    return token;
}

Token ADB_GetNextToken(Tokenizer* tokenizer) {
    Token token = ADB_PeekNextToken(tokenizer);
    Move(tokenizer, token);
    
    return token;
}

bool ADB_RequireToken(Tokenizer* tokenizer, int expectedType, Token* token) {
    if(tokenizer->parsing == false) {
        token = {};
        return false;
    }
    
    Token peek = ADB_PeekNextToken(tokenizer);
    if(token) *token = peek;
    
    if(peek.type != expectedType) {
        tokenizer->parsing = false;
        return false;
    }
    
    Move(tokenizer, peek);
    
    return true;
}

LogPriority ParsePriority(Token token) {
    switch(*token.text) {
        case 'V': return Verbose;
        case 'D': return Debug;
        case 'I': return Info;
        case 'W': return Warning;
        case 'E': return Error;
        default:  return None;
    }
}


void ParseMessage(char* message, CLArray<LogData>* buffer) {
    assert(message != NULL);
    
    buffer->Clear();
    
    Tokenizer tokenizer = {};
    tokenizer.position = message;
    tokenizer.parsing = true;
    
    
    while(tokenizer.parsing) {
        LogData logData = {};
        
        char* lineStart = tokenizer.position;
        
        // TODO: Actual string copy...
        logData.rawString = GetTextUntilEndOfLine(&tokenizer);
        tokenizer.position = lineStart;
        
        Token token = {};
        
        //- Date
        if(ADB_RequireToken(&tokenizer, Token_Number, &token)) {
            logData.date.month = (i32) GetFloat(token);
        }
        
        ADB_RequireToken(&tokenizer, Token_Dash, nullptr);
        
        if(ADB_RequireToken(&tokenizer, Token_Number, &token)) {
            logData.date.day = (i32) GetFloat(token);
        }
        
        //- Time
        if(ADB_RequireToken(&tokenizer, Token_Number, &token)) {
            logData.time.hours = (i32) GetFloat(token);
        }
        
        ADB_RequireToken(&tokenizer, Token_Colon, nullptr);
        
        if(ADB_RequireToken(&tokenizer, Token_Number, &token)) {
            logData.time.minutes = (i32) GetFloat(token);
        }
        
        ADB_RequireToken(&tokenizer, Token_Colon, nullptr);
        
        if(ADB_RequireToken(&tokenizer, Token_Number, &token)) {
            logData.time.seconds = GetFloat(token);
        }
        
        //- PID and TID
        if(ADB_RequireToken(&tokenizer, Token_Number, &token)) {
            logData.PID = (i32) GetFloat(token);
        }
        
        if(ADB_RequireToken(&tokenizer, Token_Number, &token)) {
            logData.TID = (i32) GetFloat(token);
        }
        
        //- Priority
        if(ADB_RequireToken(&tokenizer, Token_String, &token)) {
            logData.priority = ParsePriority(token);
        }
        
        //- Tag
        if(ADB_RequireToken(&tokenizer, Token_String, &token)) {
            logData.tag = GetStringFromToken(token);
        }
        
        //- Actual message
        if(ADB_RequireToken(&tokenizer, Token_Colon, &token)) {
            logData.message = GetTextUntilEndOfLine(&tokenizer);
        }
        
        ADB_RequireToken(&tokenizer, Token_EndOfLine, &token);
        
        if(tokenizer.parsing) {
            // line parse success
            buffer->Add(logData);
        }
        else {
            
        }
    }
}
