#ifndef SETTINGS_H
#define SETTINGS_H

struct Settings {
    char pathToAdb[1024]; // TODO: dynamic string?
    
    ImVec4 verboseColor;
    ImVec4 debugColor;
    ImVec4 infoColor;
    ImVec4 warningColor;
    ImVec4 errorColor;
    ImVec4 assertColor;
    
};


#endif //SETTINGS_H
