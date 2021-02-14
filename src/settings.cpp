
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
    
    fprintf(file, "path_to_adb = %s\n", settings->pathToAdb);
    
    fprintf(file, "colorVerbose = (%f, %f, %f, %f)\n", settings->verboseColor.x, settings->verboseColor.y, settings->verboseColor.z, settings->verboseColor.w);
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
