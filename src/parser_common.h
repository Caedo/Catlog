#ifndef PARSER_COMMON_H
#define PARSER_COMMON_H

enum Token_Type {
    Token_Unknown,
    
    Token_Colon,
    Token_Dash,
    Token_Equal,
    Token_Comma,
    
    Token_Number,
    Token_String,
    
    Token_OpenBracket,
    Token_CloseBracket,
    
    Token_Identifier,
    
    Token_EndOfLine,
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


inline bool IsWhiteSpace(char c) {
    return c == ' '  || c == '\t';
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

inline void StringCopy(char* dest, char* source, int length) {
    for(i32 i = 0; i < length; i++) {
        dest[i] = source[i];
    }
}

inline void EatWhiteSpaces(Tokenizer* tokenizer) {
    while(IsWhiteSpace(*tokenizer->position))
        tokenizer->position++;
}

inline void EatEndOfLine(Tokenizer* tokenizer) {
    while(IsEndOfLine(*tokenizer->position))
        tokenizer->position++;
}

i32 LineLength(char* str) {
    char* at = str;
    i32 len = 0;
    
    while(!IsEndOfLine(*at) && *at != '\0') {
        at++;
        len++;
    }
    
    return len;
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
    
    float result = (float) atof(token.text);
    return result;
}

void StringCopyTrimQuotesAndNullTerminate(char* dest, char* src, i32 srcLength) {
    if(*src == '"') {
        src++;
        srcLength--;
    }
    
    if(src[srcLength - 1] == '"') {
        srcLength--;
    }
    
    StringCopy(dest, src, srcLength);
    dest[srcLength] = 0;
}


char* GetTextUntilEndOfLine(Tokenizer* tokenizer) {
    
    while(*tokenizer->position == ' ') {
        tokenizer->position++;
    }
    
    int length = LineLength(tokenizer->position);
    
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
    while(IsEndOfLine(*at) == false && *at != '\0') {
        length++;
        at++;
    }
    
    StringCopy(destination, tokenizer->position, length);
    destination[length] = 0;
    
    tokenizer->position += length;
}


#endif //PARSER_COMMON_H
