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

#include "parser.h"

bool show_demo_window = false;

LogData* logs = NULL;

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
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

void DrawMenuBar() {
    if(ImGui::BeginMainMenuBar()) {
        if(ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New")) {}
            if (ImGui::MenuItem("Open", "Ctrl+O")) {}
            ImGui::Separator();
            if (ImGui::MenuItem("Settings")) {}
            
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

void DrawSettingsMenu() {
    
}

void DrawLogsWindow() {
    
    ImGuiID dockspaceID = ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
    ImGui::SetNextWindowDockID(dockspaceID , ImGuiCond_FirstUseEver);
    
    ImGui::Begin("Logs");
    ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Hideable | 
        ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | 
        ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg | 
        ImGuiTableFlags_ScrollY;
    
    static ImGuiTextFilter filter;
    filter.Draw("Tag Filter");
    
    static int priorityIndex;
    ImGui::Combo("Priority", &priorityIndex, LogPriorityName, IM_ARRAYSIZE(LogPriorityName));
    
    if (ImGui::BeginTable("##table1", 7, flags))
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
        
        for (int logIndex = 0; logIndex < stb_sb_count(logs); logIndex++)
        {
            LogData* log = (logs + logIndex);
            if(log->priority < priorityIndex) {
                continue;
            }
            
            if (filter.PassFilter(log->tag) == false) {
                continue;
            }
            
            
            ImGui::TableNextRow();
            if(log->priority == Warning) {
                ImU32 color = ImGui::GetColorU32(ImVec4(0.0f, 0.5f, 0.5f, 1));
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, color);
            }
            else if(log->priority == Error) {
                ImU32 color = ImGui::GetColorU32(ImVec4(0.9f, 0.1f, 0.1f, 1));
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, color);
            }
            
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
                    ImGui::Text(log->tag);
                }
            }
            
            ImGui::TableSetColumnIndex(6);
            if(log->message)
                ImGui::TextWrapped(log->message);
        }
        ImGui::EndTable();
    }
    ImGui::End();
    
}

int main()
{
    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;
    
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    
    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Dear ImGui GLFW+OpenGL3 example", NULL, NULL);
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
    
    // Our state
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    
    ProcessData pData = SpawnProcess("D:\\Projects\\C++\\Catlog\\build\\dummy_logcat.exe");
    char buffer[4096] = {};
    
    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();
        
        if(ReadProcessOut(&pData, buffer)) {
            ParserResult res = ParseMessage(buffer);
            for(int i = 0; i < res.messagesCount; i++) {
                stb_sb_push(logs, res.data[i]);
            }
        }
        
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        
        
        DrawMenuBar();
        
        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);
        
        DrawLogsWindow();
        
        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        glfwSwapBuffers(window);
    }
    
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}
