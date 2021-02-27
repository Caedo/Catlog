#include "parser_common.h"

char* LoadFileContent(const char* filePath);

Settings DefautSettings() {
    Settings settings = {};
    
    settings.verboseColor = ImVec4(1, 1, 1, 1);
    settings.debugColor   = ImVec4(1, 1, 1, 1);
    settings.infoColor    = ImVec4(1, 1, 1, 1);
    settings.warningColor = ImVec4(1, 0.92f, 0, 1);
    settings.errorColor   = ImVec4(1, 0, 0, 1);
    
    return settings;
}

ImVec4 GetColorForPriority(const Settings* settings, LogPriority priority) {
    switch(priority) {
        case Verbose: return settings->verboseColor;
        case Debug:   return settings->debugColor;
        case Info:    return settings->infoColor;
        case Warning: return settings->warningColor;
        case Error:   return settings->errorColor;
    }
    
    return ImVec4(1, 1, 1, 1);
}


void SaveSettings(const Settings* settings) {
    FILE* file = fopen("settings.data", "wb");
    assert(file);
    
    fprintf(file, "path_to_adb = \"%s\"\n", settings->pathToAdb);
    
    fprintf(file, "verboseColor = (%f, %f, %f, %f)\n", settings->verboseColor.x, settings->verboseColor.y, settings->verboseColor.z, settings->verboseColor.w);
    fprintf(file, "debugColor = (%f, %f, %f, %f)\n",   settings->debugColor.x, settings->debugColor.y, settings->debugColor.z, settings->debugColor.w);
    fprintf(file, "infoColor = (%f, %f, %f, %f)\n",    settings->infoColor.x, settings->infoColor.y, settings->infoColor.z, settings->infoColor.w);
    fprintf(file, "warningColor = (%f, %f, %f, %f)\n", settings->warningColor.x, settings->warningColor.y, settings->warningColor.z, settings->warningColor.w);
    fprintf(file, "errorColor = (%f, %f, %f, %f)\n",   settings->errorColor.x, settings->errorColor.y, settings->errorColor.z, settings->errorColor.w);
    
    fclose(file);
}

void LoadSetting(Settings* settings) {
    
    char* file = LoadFileContent("settings.data");
    if(!file) return;
    
    
    ParseSettingsFile(settings, file);
    free(file);
}

//~ Parser
Token Settings_PeekNextToken(Tokenizer* tokenizer) {
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
    else if(*at == '"') {
        token.type = Token_String;
        
        at++;
        while(*at != '"' && *at != 0) {
            at++;
        }
        at++;
        
        token.length = (i32) (at - token.text);
    }
    else if(IsAlpha(firstChar)) {
        token.type = Token_Identifier;
        
        while(IsAlpha(*at) || *at == '_') {
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
            case ',': token.type = Token_Comma; break;
            case '(': token.type = Token_OpenBracket; break;
            case ')': token.type = Token_CloseBracket; break;
            
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

Token Settings_GetNextToken(Tokenizer* tokenizer) {
    Token token = Settings_PeekNextToken(tokenizer);
    Move(tokenizer, token);
    
    return token;
}

bool Settings_RequireToken(Tokenizer* tokenizer, i32 tokenType, Token* token) {
    *token = Settings_GetNextToken(tokenizer);
    if(token->type != tokenType) {
        tokenizer->parsing = false;
        return false;
    }
    
    return true;
}

ImVec4 ParseColor(Tokenizer* tokenizer) {
    Token token = {};
    ImVec4 ret = {};
    
    Settings_RequireToken(tokenizer, Token_OpenBracket, &token);
    if(Settings_RequireToken(tokenizer, Token_Number, &token)) {
        ret.x = GetFloat(token);
    }
    
    Settings_RequireToken(tokenizer, Token_Comma, &token);
    if(Settings_RequireToken(tokenizer, Token_Number, &token)) {
        ret.y = GetFloat(token);
    }
    
    Settings_RequireToken(tokenizer, Token_Comma, &token);
    if(Settings_RequireToken(tokenizer, Token_Number, &token)) {
        ret.z = GetFloat(token);
    }
    
    token = Settings_PeekNextToken(tokenizer);
    if(token.type == Token_Comma) {
        Move(tokenizer, token);
        if(Settings_RequireToken(tokenizer, Token_Number, &token)) {
            ret.w = GetFloat(token);
        }
    }
    
    
    Settings_RequireToken(tokenizer, Token_CloseBracket, &token);
    
    return ret;
}


void ParseSettingsFile(Settings* settings, char* fileContent) {
    assert(fileContent != NULL);
    
    Tokenizer tokenizer = {};
    tokenizer.position = fileContent;
    tokenizer.parsing = true;
    
    
    while(tokenizer.parsing) {
        
        Token token = Settings_GetNextToken(&tokenizer);
        
        if(IsTokenEqual(token, "path_to_adb")) {
            token = Settings_GetNextToken(&tokenizer);
            if(token.type != Token_Equal)
                break;
            
            token = Settings_GetNextToken(&tokenizer);
            if(token.type == Token_String) {
                StringCopyTrimQuotesAndNullTerminate(settings->pathToAdb, token.text, token.length);
            }
            else {
                break;
            }
        }
        else if(IsTokenEqual(token, "verboseColor")) {
            Settings_RequireToken(&tokenizer, Token_Equal, &token);
            
            settings->verboseColor = ParseColor(&tokenizer);
        }
        else if(IsTokenEqual(token, "debugColor")) {
            Settings_RequireToken(&tokenizer, Token_Equal, &token);
            
            settings->debugColor = ParseColor(&tokenizer);
        }
        else if(IsTokenEqual(token, "infoColor")) {
            Settings_RequireToken(&tokenizer, Token_Equal, &token);
            
            settings->infoColor = ParseColor(&tokenizer);
        }
        else if(IsTokenEqual(token, "warningColor")) {
            Settings_RequireToken(&tokenizer, Token_Equal, &token);
            
            settings->warningColor = ParseColor(&tokenizer);
        }
        else if(IsTokenEqual(token, "errorColor")) {
            Settings_RequireToken(&tokenizer, Token_Equal, &token);
            
            settings->errorColor = ParseColor(&tokenizer);
        }
    }
}
