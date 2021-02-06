
char* LoadFileContent(const char* filePath);

ImVec4 GetColorForPriority(const Settings* settings, LogPriority priority) {
    switch(priority) {
        case Verbose: return settings->verboseColor;
        case Debug:   return settings->debugColor;
        case Info:    return settings->infoColor;
        case Warning: return settings->warningColor;
        case Error:   return settings->errorColor;
        case Assert:  return settings->assertColor;
    }
    
    return ImVec4();
}


void SaveSettings(const Settings* settings) {
    FILE* file = fopen("settings.data", "wb");
    assert(file);
    
    fprintf(file, "path_to_adb = %s", settings->pathToAdb);
    fclose(file);
}

Settings LoadSetting() {
    Settings settings = {};
    
    char* file = LoadFileContent("settings.data");
    if(!file)
        return settings;
    
    
    ParseSettingsFile(&settings, file);
    free(file);
    
    return settings;
}
