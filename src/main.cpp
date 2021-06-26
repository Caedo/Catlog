#include <stdio.h>

#ifdef MTR_ENABLED
    #include "Minitrace/minitrace.h"
    #include "Minitrace/minitrace.c"
#else
    #define MTR_BEGIN
    #define MTR_END
    #define MTR_SCOPE
#endif // MTR_ENABLED

#include "glad.c"

#include "ImGUI/imgui.cpp"
#include "ImGUI/imgui_draw.cpp"
#include "ImGUI/imgui_tables.cpp"
#include "ImGUI/imgui_widgets.cpp"
#include "ImGUI/imgui_impl_glfw.cpp"
#include "ImGUI/imgui_impl_opengl3.cpp"

#include "ImGUI/imgui_demo.cpp"

#define GLFW_DLL
#define GLFW_INCLUDE_NONE

#include <glad/glad.h>
#include <GLFW/glfw3.h>


#include "common.h"
#include "CLArray.h"
#include "CLstr.h"

#include "parser_common.h"
#include "platform_win32.h"
#include "settings.h"

#include "logcat_parser.cpp"
#include "platform_win32.cpp"
#include "settings.cpp"


enum LogcatBufferFlags_ {
    LogcatBufferFlags_None   = 0,
    LogcatBufferFlags_Radio  = 1 << 0,
    LogcatBufferFlags_Events = 1 << 1,
    LogcatBufferFlags_Main   = 1 << 2,
    LogcatBufferFlags_System = 1 << 3,
    LogcatBufferFlags_Crash  = 1 << 4,

    LogcatBufferFlags_All     = (LogcatBufferFlags_Radio | LogcatBufferFlags_Events | LogcatBufferFlags_Main | LogcatBufferFlags_System | LogcatBufferFlags_Crash),

    // NOTE: According to the documentation, those are default buffers. They can be ommited, but it is easier for me to handle them explicitly
    LogcatBufferFlags_Default = (LogcatBufferFlags_Main | LogcatBufferFlags_System | LogcatBufferFlags_Crash), 
};

typedef int LogcatBufferFlags;

struct TagPriorityPair {
    char tag[64];
    LogPriority priority;
};

struct WindowElements {
    // ImGui stuff
    int id;
    char label[128];
    bool isOpen;
    
    bool isOpennedWithFile;
    char deviceId[128];
    
    // Logs
    CLArray<LogData> logs;
    CLArray<int> filteredLogIndices;
    CLArray<TagPriorityPair> tags;
    
    // Filter things
    bool filtersActive;
    ImGuiTextFilter tagFilter;
    ImGuiTextFilter messageFilter;
    int priorityIndex;

    LogcatBufferFlags bufferFlags;
    
    // TODO: change to opaque pointer or other cross-platform stuff
    ProcessData process;
};

bool show_demo_window = false;
bool showSettingsWindow = false;

Settings settings = {};

// NOTE: TODO: Do we really need dynamic array here...?
CLArray<WindowElements> windowsData = {};

void OpenNewWindow() {
    static int nextWindowId = 0;
    
    windowsData.AddEmpty();
    WindowElements* window = windowsData.data + windowsData.count - 1;
    
    window->id = nextWindowId++;
    window->isOpen = true;
    
    snprintf(window->label, IM_ARRAYSIZE(window->label), "Logs###%d", window->id);
}

void ClearWindow(WindowElements* window) {
    CloseProcess(&window->process);
    
    window->logs.Free();
    window->filteredLogIndices.Free();
    window->tags.Free();
}

char* LoadFileContent(const char* filePath) {
    char* ret = NULL;
    
    FILE* file = fopen(filePath, "rb");
    if(file) {
        fseek(file, 0, SEEK_END);
        long length = ftell(file);
        fseek(file, 0, SEEK_SET);
        
        ret = (char*) malloc((length + 1) * sizeof(char));
        fread(ret, 1, length, file);
        ret[length] = '\0';
        
        fclose(file);
    }
    
    return ret;
}

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}


char PriorityToChar(LogPriority priority) {
    switch(priority) {
        case None: return 'N';
        case Verbose: return 'V';
        case Debug: return 'D';
        case Info: return 'I';
        case Warning: return 'W';
        case Error: return 'E';
        case Fatal: return 'F';
        case Silent: return 'S';
    }
    
    return '?';
}

bool PassesWindowFilters(WindowElements* elements, LogData* log) {
    return elements->tagFilter.PassFilter(log->tag) &&
        elements->messageFilter.PassFilter(log->message) &&
        log->priority >= elements->priorityIndex;
}

void DrawSettingsMenu() {
    ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_Once);
    ImGui::Begin("Settings Window", &showSettingsWindow);
    
    ImGui::BeginChild("Tags", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 5)); 
    if(ImGui::BeginTabBar("tabs", ImGuiTabBarFlags_None)) {
        if(ImGui::BeginTabItem("ADB")) {
            ImGui::Text("Path to ADB: %s", settings.pathToAdb);
            
            ImGui::SameLine();
            if(ImGui::Button("Browse...")) {
                OpenFileDialog(settings.pathToAdb, IM_ARRAYSIZE(settings.pathToAdb));
            }
            
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("Colors")) {
            ImGuiColorEditFlags flags = ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_Float; 
            
            ImGui::ColorEdit4("Verbose", (float*) &settings.verboseColor, flags);
            ImGui::ColorEdit4("Debug", (float*) &settings.debugColor, flags);
            ImGui::ColorEdit4("Info", (float*) &settings.infoColor, flags);
            ImGui::ColorEdit4("Warning", (float*) &settings.warningColor, flags);
            ImGui::ColorEdit4("Error", (float*) &settings.errorColor, flags);
            
            ImGui::EndTabItem();
        }
        
        ImGui::EndTabBar();
    }
    
    ImGui::EndChild();
    ImGui::Separator();

    if(ImGui::Button("Save")) {
        SaveSettings(&settings);
    }

    ImGui::SameLine();
    if(ImGui::Button("Save & Exit")) {
        SaveSettings(&settings);
        showSettingsWindow = false;
    }

    ImGui::End();
}

void DrawMenuBar() {
    if(ImGui::BeginMainMenuBar()) {
        if(ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New")) {}
            if (ImGui::MenuItem("Open", "Ctrl+O")) {
                
                char path[1024];
                CLStr pathStr = StrMakeBuffer(path, IM_ARRAYSIZE(path));
                
                char fileName[256];
                CLStr fileNameStr = StrMakeBuffer(fileName, IM_ARRAYSIZE(fileName));
                
                OpenFileDialog(&pathStr, &fileNameStr);
                
                if (path[0] != 0) {
                    WindowElements* windowElements = windowsData.data;
                    windowElements->logs.Free();
                    
                    char* buffer = LoadFileContent(path);
                    static CLArray<LogData> parsedLogs = {};
                    ParseMessage(buffer, &parsedLogs);
                    free(buffer);
                    
                    for (int i = 0; i < parsedLogs.count; i++) {
                        windowElements->logs.Add(parsedLogs.data[i]);
                    }
                    
                    windowElements->isOpennedWithFile = true;
                    snprintf(windowElements->label, IM_ARRAYSIZE(windowElements->label), "%s###%d", fileName, windowElements->id); 
                }
            }
            if (ImGui::MenuItem("Save")) {
                char path[1024];
                CLStr pathStr = StrMakeBuffer(path, IM_ARRAYSIZE(path));
                
                SaveFileDialog(&pathStr);
                
                if(path[0] != 0) {
                    FILE* file = fopen(path, "wb");
                    WindowElements *windowElements = windowsData.data;
                    for (int i = 0; i < windowElements->logs.count; i++) {
                        LogData *log = windowElements->logs.data + i;
                        fprintf(file, log->rawString);
                        fprintf(file, "\n");
                    }

                    fclose(file);
                }
                
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Settings")) {
                showSettingsWindow = !showSettingsWindow;
            }
            
            ImGui::EndMenu();
        }
        
        if(ImGui::BeginMenu("Window")) {
            if(ImGui::MenuItem("New Window")) {
                OpenNewWindow();
            }
            ImGui::EndMenu();
        }
        
        
        if(ImGui::BeginMenu("Help")) {
            if(ImGui::MenuItem("Show/Hide ImGui help")) {
                show_demo_window = !show_demo_window;
            }
            
            ImGui::EndMenu();
        }
        
        ImGui::EndMainMenuBar();
    }
}

void DrawSetADBPathPopup() {
    ImGui::SetNextWindowSize(ImVec2(450, 250), ImGuiCond_Once);
    if(ImGui::BeginPopupModal("Set ADB Path")) {
        ImGui::BeginChild("Please, set the path :>", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 5)); 
        
        ImGui::Text("Please set path to your adb installation.");
        ImGui::Text("Path to ADB: %s", settings.pathToAdb);
        
        if(ImGui::Button("Browse...")) {
            OpenFileDialog(settings.pathToAdb, IM_ARRAYSIZE(settings.pathToAdb));
        }
        
        ImGui::EndChild();
        
        ImGui::Separator();
        if(settings.pathToAdb && settings.pathToAdb[0] != 0) {
            if(ImGui::Button("Save and Continue")) {
                SaveSettings(&settings);
                
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::SameLine();
        }
        
        
        if(ImGui::Button("Close")) {
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
}

void DrawProcessCreateFailedPopup() {
    if(ImGui::BeginPopupModal("Create Failed", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Failed to create ADB process!");
        ImGui::Text("Please make sure that your path to ADB executable is correct.");
        
        ImGui::Separator();
        if(ImGui::Button("Close")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void DrawLogcatParametersPopup(WindowElements* windowElements) {

    ImGui::SetNextWindowSize(ImVec2(475, 200), ImGuiCond_Once);
    if(ImGui::BeginPopupModal("Logcat Parameters")) {
        
        ImGui::BeginChild("Tabs", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 5)); 
        if(ImGui::BeginTabBar("Logcat Params Tab")) {
            if(ImGui::BeginTabItem("Tags")) {
                ImGui::PushItemWidth(ImGui::GetFontSize() * 12);
                {
                    for(int i = 0; i < windowElements->tags.count; i++) {
                        ImGui::PushID(i);
                        
                        ImGui::InputText("Tag", windowElements->tags[i].tag, 64);
                        ImGui::SameLine();
                        
                        if (ImGui::BeginCombo("Priority", LogPriorityName[(int)(windowElements->tags.data[i].priority) ]))
                        {
                            // we want to skip "None" priority
                            for (int n = 1; n < IM_ARRAYSIZE(LogPriorityName); n++)
                            {
                                const bool is_selected = ((int) (windowElements->tags[i].priority == n));
                                if (ImGui::Selectable(LogPriorityName[n], is_selected)) {
                                    windowElements->tags.data[i].priority = (LogPriority) n;
                                }
                                
                                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                                if (is_selected) {
                                    ImGui::SetItemDefaultFocus();
                                }
                                
                            }
                            ImGui::EndCombo();
                        }
                        
                        ImGui::SameLine();
                        if(ImGui::Button("X")) {
                            windowElements->tags.RemoveAt(i);
                        }
                        
                        ImGui::PopID();
                    }
                }
                ImGui::PopItemWidth();
                
                if(ImGui::Button("Add tag")) {
                    TagPriorityPair newPair = {};
                    newPair.priority = Verbose;
                    
                    windowElements->tags.Add(newPair);
                }
                
                ImGui::EndTabItem();
            }
            
            if(ImGui::BeginTabItem("Buffers")) {
                
                ImGui::Text("Please select desired adb logcat buffers");
                ImGui::PushItemWidth(ImGui::GetFontSize() * 12);
                {
                    if(windowElements->bufferFlags == 0) {
                        windowElements->bufferFlags = LogcatBufferFlags_Default;
                    }

                    if(ImGui::Selectable("Default", windowElements->bufferFlags == LogcatBufferFlags_Default)) {
                        windowElements->bufferFlags = LogcatBufferFlags_Default;
                    }

                    if(ImGui::Selectable("All", windowElements->bufferFlags == LogcatBufferFlags_All)) {
                        windowElements->bufferFlags = LogcatBufferFlags_All;
                    }

                    ImGui::Separator();
                    if(ImGui::Selectable("Radio", windowElements->bufferFlags & LogcatBufferFlags_Radio)) {
                        windowElements->bufferFlags ^= LogcatBufferFlags_Radio;
                    } 

                    if(ImGui::Selectable("Events", windowElements->bufferFlags & LogcatBufferFlags_Events)) {
                        windowElements->bufferFlags ^= LogcatBufferFlags_Events;
                    }

                    if(ImGui::Selectable("Main", windowElements->bufferFlags & LogcatBufferFlags_Main)) {
                        windowElements->bufferFlags ^= LogcatBufferFlags_Main;
                    }

                    if(ImGui::Selectable("System", windowElements->bufferFlags & LogcatBufferFlags_System)) {
                        windowElements->bufferFlags ^= LogcatBufferFlags_System;
                    }

                    if(ImGui::Selectable("Crash", windowElements->bufferFlags & LogcatBufferFlags_Crash)) {
                        windowElements->bufferFlags ^= LogcatBufferFlags_Crash;
                    }
                }
                ImGui::PopItemWidth();
                
                ImGui::EndTabItem();
            }
            
            ImGui::EndTabBar();
        }
        
        ImGui::EndChild();
        ImGui::Separator();
        
        if(ImGui::Button("Ok")) {
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
}

void DrawLogsWindow(WindowElements* windowElements) {
    ImGui::Begin(windowElements->label, &windowElements->isOpen);


    ImGui::BeginChild("left pane", ImVec2(90, 0), true);

    DrawProcessCreateFailedPopup();
    DrawSetADBPathPopup();
    DrawLogcatParametersPopup(windowElements);

    if(windowElements->isOpennedWithFile == false) {
        if(windowElements->process.isRunning == false) {
            if(ImGui::Button("Start")) {
                if(settings.pathToAdb == NULL || settings.pathToAdb[0] == 0) {
                    ImGui::OpenPopup("Set ADB Path");
                }
                else {
                    ImGuiTextBuffer strBuffer = {};
                    
                    strBuffer.appendf("%s logcat", settings.pathToAdb);
                    if(windowElements->tags.count > 0) {
                        // Set all tags to silent, so specified tags will be displayed
                        strBuffer.append(" *:S");
                    }
                    
                    for(int i = 0; i < windowElements->tags.count; i++) {
                        strBuffer.appendf("%s:%c", windowElements->tags[i].tag, PriorityToChar(windowElements->tags[i].priority));
                    }

                    if(windowElements->bufferFlags == LogcatBufferFlags_All) {
                        strBuffer.append(" -b all ");
                    }
                    else {
                        // As the documentation seems invalid, all buffers have to be preceeded by -b
                        if(windowElements->bufferFlags & LogcatBufferFlags_Radio) {
                            strBuffer.append(" -b radio "); 
                        }
                        if(windowElements->bufferFlags & LogcatBufferFlags_Events) {
                            strBuffer.append(" -b events ");
                        }
                        if(windowElements->bufferFlags & LogcatBufferFlags_Main) {
                            strBuffer.append(" -b main ");
                        }
                        if(windowElements->bufferFlags & LogcatBufferFlags_System) {
                            strBuffer.append(" -b system ");
                        }
                        if(windowElements->bufferFlags & LogcatBufferFlags_Crash) {
                            strBuffer.append(" -b crash ");
                        }
                    }
                    
                    printf("%s\n", strBuffer.c_str());
                    
                    windowElements->process = SpawnProcess((char*) strBuffer.c_str());
                    
                    if(!windowElements->process.isRunning) {
                        ImGui::OpenPopup("Create Failed");
                    }
                }
            }
        }
        else {
            if(ImGui::Button("Stop")) {
                if(windowElements->process.pinfo.hProcess) {
                    CloseProcess(&windowElements->process);
                }
            }
        }

        if(ImGui::Button("Parameters")) {
            ImGui::OpenPopup("Logcat Parameters");
        }
        
        ImGui::Separator();
    }

    ImGui::Text("Count: %d", windowElements->logs.count);

    if(ImGui::Button("Clear logs")) {

        for(int i = 0; i < windowElements->logs.count; i++) {
            free(windowElements->logs[i].message);
            free(windowElements->logs[i].tag);
        }

        windowElements->logs.Clear();
        windowElements->filteredLogIndices.Clear();
    }

    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::BeginChild("Table");

    ImGui::PushItemWidth(ImGui::GetFontSize() * 12);
    {
        bool filtersEdited = false;
        
        windowElements->tagFilter.Draw("Tag Filter");
        filtersEdited = filtersEdited || ImGui::IsItemEdited();
        
        ImGui::SameLine();
        windowElements->messageFilter.Draw("Message Filter");
        filtersEdited = filtersEdited || ImGui::IsItemEdited();
        
        ImGui::SameLine();
        ImGui::Combo("Priority", &windowElements->priorityIndex, LogPriorityName, IM_ARRAYSIZE(LogPriorityName));
        filtersEdited = filtersEdited || ImGui::IsItemEdited();
        
        // NOTE: TODO:
        // This can be slow when there will be a lot of elements
        // should be profilled and changed if needed
        if(filtersEdited) {
            
            windowElements->filtersActive = windowElements->tagFilter.IsActive() || windowElements->messageFilter.IsActive() || windowElements->priorityIndex > 0;
            
            if(windowElements->filtersActive) {
                windowElements->filteredLogIndices.Clear();
                
                for(int i = 0; i < windowElements->logs.count; i++) {
                    LogData* log = windowElements->logs.data + i;
                    if(PassesWindowFilters(windowElements, log)) {
                        windowElements->filteredLogIndices.Add(i);
                    }
                }
            }
        }
    }
    ImGui::PopItemWidth();
    
    if(ImGui::Button("Clear filters")) {
        windowElements->filteredLogIndices.Clear();
        
        windowElements->filtersActive = false;
        windowElements->tagFilter.Clear();
        windowElements->messageFilter.Clear();
        windowElements->priorityIndex = 0;
    }



    ImGuiTableFlags tableFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Hideable | 
        ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | 
        ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg | 
        ImGuiTableFlags_ScrollY;
    
    MTR_BEGIN("main", "table render");
    if (ImGui::BeginTable("##table1", 7, tableFlags))
    {
        
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("Date");
        ImGui::TableSetupColumn("Time");
        ImGui::TableSetupColumn("PID");
        ImGui::TableSetupColumn("TID");
        ImGui::TableSetupColumn("Priority");
        ImGui::TableSetupColumn("Tag");
        ImGui::TableSetupColumn("Message", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();
        
        bool filtersActive = windowElements->filtersActive;
        int elementsCount = filtersActive ? windowElements->filteredLogIndices.count : windowElements->logs.count;
        
        ImGuiListClipper clipper;
        clipper.Begin(elementsCount);
        
        while(clipper.Step())
        for (int logIndex = clipper.DisplayStart; logIndex < clipper.DisplayEnd; logIndex++)
        {
            u32 index = filtersActive ? windowElements->filteredLogIndices[logIndex] : logIndex;
            LogData* log = windowElements->logs.data + index;
            
            ImGui::TableNextRow();
            
            ImVec4 color = GetColorForPriority(&settings, log->priority);
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(color));
            
            if(log->parseFailed == false) {
                ImGui::TableSetColumnIndex(0);
                if(log->date.year == 0) {
                    ImGui::Text("%d-%d", log->date.day, log->date.month);
                }
                else {
                    ImGui::Text("%d-%d-%d", log->date.day, log->date.month, log->date.year);
                }
                
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%d:%d:%.2f", log->time.hours, log->time.minutes, log->time.seconds);
                
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%d", log->PID);
                
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%d", log->TID);
                
                ImGui::TableSetColumnIndex(4);
                ImGui::Text("%s", LogPriorityName[log->priority]);
                
                if(log->tag) {
                    ImGui::TableSetColumnIndex(5);
                    ImGui::Text("%s", log->tag);
                }
            }
            
            ImGui::TableSetColumnIndex(6);
            if(log->message) {
                ImGui::TextUnformatted(log->message);
                
                float textSize = ImGui::GetItemRectSize().x;
                float columnSize = ImGui::GetContentRegionAvail().x;
                if(ImGui::IsItemHovered() && textSize > columnSize) {
                    
                    ImGui::BeginTooltip();
                    
                    ImGui::PushTextWrapPos(columnSize);
                    ImGui::TextWrapped(log->message);
                    ImGui::PopTextWrapPos();
                    
                    ImGui::EndTooltip();
                }
            }
            
            ImGui::PopStyleColor();
        }
        
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }
        
        ImGui::EndTable();
    }

    ImGui::EndChild();
    ImGui::EndGroup();

    MTR_END("main", "table render");
    ImGui::End();

}

int main()
{
#ifdef MTR_ENABLED
    mtr_init("trace.json");
    MTR_META_PROCESS_NAME("Catlog");
    MTR_META_THREAD_NAME("main thread");
#endif

    OpenNewWindow();
    
    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;
    
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    
    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Catlog", NULL, NULL);
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
    
    if (gladLoadGL() == 0)
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return 1;
    }
    
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();
    
    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    
    settings = DefautSettings();
    LoadSetting(&settings);
    
    // Our state
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    
    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();
        
        MTR_BEGIN("main", "render");
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        DrawMenuBar();
        
        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);
        
        ImGuiID dockspaceID = ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
        
        for(int i = 0; i < windowsData.count; i++) {
            WindowElements* windowData = windowsData.data + i;
            
            MTR_BEGIN("main", "read adb out");
            char buffer[8192];
            if(windowData->process.pinfo.hProcess && ReadProcessOut(&windowData->process, buffer, IM_ARRAYSIZE(buffer))) {
                static CLArray<LogData> parsedLogs = {};
                
                MTR_BEGIN("main", "parsing");
                ParseMessage(buffer, &parsedLogs);
                MTR_END("main", "parsing");
                
                MTR_BEGIN("main", "logs push");
                for(int j = 0; j < parsedLogs.count; j++) {
                    // TODO: NOTE: Maybe just pass actual buffer without copying?
                    windowData->logs.Add(parsedLogs.data[j]);
                    
                    if(windowData->filtersActive && PassesWindowFilters(windowData, &parsedLogs.data[j])) {
                        windowData->filteredLogIndices.Add(windowData->logs.count - 1);
                    }
                }
                
                MTR_END("main", "logs push");
            }
            MTR_END("main", "read adb out");
            
            ImGui::SetNextWindowDockID(dockspaceID , ImGuiCond_Once);
            DrawLogsWindow(windowData);
        }
        
        
        for(int i = windowsData.count - 1; i >= 0; i--) {
            WindowElements* windowData = windowsData.data + i;
            
            if(windowData->isOpen == false) {
                ClearWindow(windowData);
                windowsData.RemoveAt(i);
            }
        }
        
        if(showSettingsWindow)
            DrawSettingsMenu();
        
        
        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        glfwSwapBuffers(window);
        MTR_END("main", "render");
    }
    
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    for(int i = 0; i < windowsData.count; i++) {
        CloseProcess(&windowsData[i].process);
    }
    
    glfwDestroyWindow(window);
    glfwTerminate();
    
#if MTR_ENABLED
    mtr_flush();
    mtr_shutdown();
#endif
    
    return 0;
}
