#include "parser.h"

#define MAX_MESSAGES 10

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
    tokenizer->position = token.text + token.length;
}

void EatWhiteSpaces(Tokenizer* tokenizer) {
    while(IsWhiteSpace(*tokenizer->position))
        tokenizer->position++;
}

Token PeekNextToken(Tokenizer* tokenizer) {
    Token token = {};
    
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
    else if(IsAlpha(firstChar)) {
        token.type = Token_String;
        
        while(IsAlpha(*at)) {
            at++;
        }
    }
    else {
        switch(firstChar) {
            case '-': token.type = Token_Dash; break;
            case ':': token.type = Token_Semicolon; break;
            case '/': token.type = Token_Slash; break;
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


LogData* ParseMessage(char* message, int* messagesCount) {
    LogData* ret = (LogData*) malloc(MAX_MESSAGES * sizeof(LogData));
    
    Tokenizer tokenizer = {};
    tokenizer.position = message;
    tokenizer.parsing = true;
    
    while(tokenizer.parsing) {
        
        Token t1 = RequireToken(&tokenizer, Token_Number);
        Token t2 = RequireToken(&tokenizer, Token_Dash);
        Token t3 = RequireToken(&tokenizer, Token_Number);
    }
    
    
    
    
    return ret;
}