#include <stdio.h>

#include "stretchy_buffer.h"

#include "platform_win32.h"

#define GLFW_DLL
#define GLFW_INCLUDE_NONE

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "ImGUI/imgui.h"
#include "ImGUI/imgui_impl_glfw.h"
#include "ImGUI/imgui_impl_opengl3.h"

#include "settings.h"

#include "parser.h"

bool show_demo_window = false;
bool showSettingsWindow = false;

LogData* logs = NULL;

Settings settings = {};
ProcessData process = {};

CL_Array<TagPriorityPair> tags;

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

void DrawSettingsMenu() {
    ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_Once);
    ImGui::Begin("Settings Window", &showSettingsWindow);
    
    ImGui::BeginChild("Tags", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 5)); 
    if(ImGui::BeginTabBar("tabs", ImGuiTabBarFlags_None)) {
        if (ImGui::BeginTabItem("ADB")) {
            ImGui::Text("Path to ADB: %s", settings.pathToAdb);
            
            ImGui::SameLine();
            if(ImGui::Button("Browse...")) {
                OpenFileDialog(settings.pathToAdb, 1024);
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
    
    ImGui::End();
}

void DrawMenuBar() {
    if(ImGui::BeginMainMenuBar()) {
        if(ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New")) {}
            if (ImGui::MenuItem("Open", "Ctrl+O")) {}
            ImGui::Separator();
            if (ImGui::MenuItem("Settings")) {
                showSettingsWindow = !showSettingsWindow;
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

void DrawLogsWindow() {
    
    ImGuiID dockspaceID = ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
    ImGui::SetNextWindowDockID(dockspaceID , ImGuiCond_FirstUseEver);
    
    ImGui::Begin("Logs");
    
    static ImGuiTextFilter tagFilter;
    static ImGuiTextFilter messageFilter;
    static int priorityIndex;
    
    if(process.isRunning == false) {
        if(ImGui::Button("Start")) {
            if(settings.pathToAdb && settings.pathToAdb[0] != 0) {
                ImGui::OpenPopup("Logcat Parameters");
            }
            else {
                ImGui::OpenPopup("Set ADB Path");
            }
        }
    }
    else {
        if(ImGui::Button("Stop")) {
            if(process.pinfo.hProcess) {
                CloseProcess(&process);
            }
        }
    }
    
    ImGui::SameLine();
    if(ImGui::Button("Clear")) {
        stb_sb_free(logs);
        logs = NULL;
    }
    
    ImGui::Separator();
    
    
    bool shouldOpenPopup = false;
    ImGui::SetNextWindowSize(ImVec2(450, 250), ImGuiCond_Once);
    if(ImGui::BeginPopupModal("Set ADB Path")) {
        ImGui::BeginChild("Please, set the path :>", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 5)); 
        
        ImGui::Text("Please set path to your adb installation.");
        ImGui::Text("Path to ADB: %s", settings.pathToAdb);
        
        if(ImGui::Button("Browse...")) {
            OpenFileDialog(settings.pathToAdb, 1024);
        }
        
        ImGui::EndChild();
        
        ImGui::Separator();
        if(settings.pathToAdb && settings.pathToAdb[0] != 0) {
            if(ImGui::Button("Save and Continue")) {
                SaveSettings(&settings);
                
                ImGui::CloseCurrentPopup();
                shouldOpenPopup = true;
            }
            
            ImGui::SameLine();
        }
        
        
        if(ImGui::Button("Close")) {
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
    
    if(shouldOpenPopup) {
        ImGui::OpenPopup("Logcat Parameters");
    }
    
    ImGui::SetNextWindowSize(ImVec2(475, 200), ImGuiCond_Once);
    if(ImGui::BeginPopupModal("Logcat Parameters")) {
        
        ImGui::BeginChild("Tags", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 5)); 
        ImGui::PushItemWidth(ImGui::GetFontSize() * 12);
        {
            for(int i = 0; i < tags.count; i++) {
                ImGui::PushID(i);
                
                ImGui::InputText("Tag", tags.data[i].tag, 64);
                ImGui::SameLine();
                
                if (ImGui::BeginCombo("Priority", LogPriorityName[(int)tags.data[i].priority ]))
                {
                    // we want to skip "None" priority
                    for (int n = 1; n < IM_ARRAYSIZE(LogPriorityName); n++)
                    {
                        const bool is_selected = ((int) tags.data[n].priority == n);
                        if (ImGui::Selectable(LogPriorityName[n], is_selected)) {
                            tags.data[i].priority = (LogPriority) n;
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
                    tags.RemoveAt(i);
                }
                
                ImGui::PopID();
            }
        }
        ImGui::PopItemWidth();
        
        if(ImGui::Button("Add tag")) {
            TagPriorityPair newPair = {};
            newPair.priority = Verbose;
            
            tags.Add(newPair);
        }
        
        ImGui::EndChild();
        
        
        ImGui::Separator();
        if(ImGui::Button("Start")) {
            if(settings.pathToAdb[0] != '\0')  {
                i32 pathLen = (i32) strlen(settings.pathToAdb);
                pathLen += (i32) strlen(" logcat *:S");
                
                for(int i = 0; i < tags.count; i++) {
                    pathLen += (i32) strlen(tags[i].tag) + 3;
                }
                
                char* proc = (char*) malloc(pathLen + 1);
                sprintf(proc, "%s logcat", settings.pathToAdb);
                if(tags.count > 0) {
                    strcat(proc, " *:S");
                }
                
                for(int i = 0; i < tags.count; i++) {
                    char buff[70];
                    sprintf(buff, " %s:%c", tags[i].tag, PriorityToChar(tags[i].priority));
                    
                    strcat(proc, buff);
                }
                
                process = SpawnProcess(proc);
                free(proc);
            }
            
            if(!process.isRunning) {
                ImGui::OpenPopup("Create Failed");
            }
            else {
                ImGui::CloseCurrentPopup();
            }
        }
        
        if(ImGui::BeginPopupModal("Create Failed", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            
            ImGui::Text("Failed to create ADB process!");
            ImGui::Text("Please make sure that your path to ADB executable is correct.");
            
            ImGui::Separator();
            if(ImGui::Button("Close")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        
        ImGui::SameLine();
        if(ImGui::Button("Close")) {
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
    
    
    ImGui::PushItemWidth(ImGui::GetFontSize() * 12);
    {
        tagFilter.Draw("Tag Filter");
        
        ImGui::SameLine();
        messageFilter.Draw("Message Filter");
        
        ImGui::SameLine();
        ImGui::Combo("Priority", &priorityIndex, LogPriorityName, IM_ARRAYSIZE(LogPriorityName));
    }
    ImGui::PopItemWidth();
    ImGui::Text("Count: %d", stb_sb_count(logs));
    
    ImGuiTableFlags tableFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Hideable | 
        ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | 
        ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg | 
        ImGuiTableFlags_ScrollY | ImGuiTableFlags_ScrollX;
    
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
        
        ImGuiListClipper clipper;
        clipper.Begin(stb_sb_count(logs));
        
        while(clipper.Step())
            for (int logIndex = clipper.DisplayStart; logIndex < clipper.DisplayEnd; logIndex++)
        {
            LogData* log = (logs + logIndex);
            if(log->priority < priorityIndex) {
                continue;
            }
            
            if (tagFilter.PassFilter(log->tag) == false) {
                continue;
            }
            if (messageFilter.PassFilter(log->message) == false) {
                continue;
            }
            
            
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
                    ImGui::Text("Column: %.3f  Text: %.3f", columnSize, textSize);
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
        
        MTR_BEGIN("main", "read adb out");
        char buffer[8192];
        if(process.pinfo.hProcess && ReadProcessOut(&process, buffer)) {
            static CL_Array<LogData> parsedLogs = {};
            
            MTR_BEGIN("main", "parsing");
            ParseMessage(buffer, &parsedLogs);
            MTR_END("main", "parsing");
            
            MTR_BEGIN("main", "logs push");
            for(int i = 0; i < parsedLogs.count; i++) {
                stb_sb_push(logs, parsedLogs.data[i]);
            }
            
            MTR_END("main", "logs push");
        }
        MTR_END("main", "read adb out");
        
        MTR_BEGIN("main", "render");
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        DrawMenuBar();
        
        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);
        
        DrawLogsWindow();
        
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
    
    CloseProcess(&process);
    
    glfwDestroyWindow(window);
    glfwTerminate();
    
#if MTR_ENABLED
    mtr_flush();
    mtr_shutdown();
#endif
    
    return 0;
}
