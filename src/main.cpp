
#include "ImGUI/imgui.h"
#include "ImGUI/imgui_impl_glfw.h"
#include "ImGUI/imgui_impl_opengl3.h"
#include <stdio.h>

#define GLFW_DLL
#define GLFW_INCLUDE_NONE


#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "parser.h"

#include "glad.c"
#include "ImGUI/imgui.cpp"
#include "ImGUI/imgui_draw.cpp"
#include "ImGUI/imgui_tables.cpp"
#include "ImGUI/imgui_widgets.cpp"
#include "ImGUI/imgui_impl_glfw.cpp"
#include "ImGUI/imgui_impl_opengl3.cpp"

#include "ImGUI/imgui_demo.cpp"

char testMessage[] = "12-10 13:02:50.071 1901-4229/com.google.android.gms V/AuthZen: Handling delegate intent. \n 13-05 11:12:45.071 1902-4219/com.google.android.gms D/Unity: Handling sdfasdf daf d. \n \
13-05 11:12:45.071 1902-4219/com.google.android.gms W/Unity: Handling sdfasdf daf d.\n \
13-05 11:12:45.071 1902-4219/com.google.android.gms E/Unity: Handling sdfasdf daf d.";

struct cl_string {
    char* data;
    int length;
};

struct Date {
    int year;
    int month;
    int day;
};

struct Time {
    int hours;
    int minutes;
    float seconds;
};

enum LogPriority {
    None = 0, // debug value
    Verbose = 'V',
    Debug = 'D',
    Info = 'I',
    Warning = 'W',
    Error = 'E',
    Assert = 'A',
    Fatal = 'F',
    Silent = 'S'
};

struct LogData {
    Date date;
    Time time;
    int PID;
    int TID;
    
    LogPriority priority;
    
    char* package;
    char* tag;
    char* message;
};

#include "parser.cpp"



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
            
            ImGui::EndMenu();
        }
        
        ImGui::EndMainMenuBar();
    }
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
    
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();
    
    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    
    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    
    char* file = LoadFileContent("testmessage_2.txt");
    
    int messagesCount;
    LogData* logs = ParseMessage(file, &messagesCount);
    
    
    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();
        
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        DrawMenuBar();
        
        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);
        
        
        ImGui::Begin("Logs");
        ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Hideable | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY;
        if (ImGui::BeginTable("##table1", 8, flags))
        {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("Date");
            ImGui::TableSetupColumn("Time");
            ImGui::TableSetupColumn("PID");
            ImGui::TableSetupColumn("TID");
            ImGui::TableSetupColumn("Package");
            ImGui::TableSetupColumn("Priority");
            ImGui::TableSetupColumn("Tag");
            ImGui::TableSetupColumn("Message", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();
            
            for (int logIndex = 0; logIndex < messagesCount; logIndex++)
            {
                ImGui::TableNextRow();
                LogData* log = (logs + logIndex);
                
                if(log->priority == Warning) {
                    ImU32 color = ImGui::GetColorU32(ImVec4(0.0f, 0.5f, 0.5f, 1));
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, color);
                }
                else if(log->priority == Error) {
                    ImU32 color = ImGui::GetColorU32(ImVec4(0.9f, 0.1f, 0.1f, 1));
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, color);
                }
                
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%d-%d-%d", log->date.day, log->date.month, log->date.year);
                
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%d:%d:%.2f", log->time.hours, log->time.minutes, log->time.seconds);
                
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%d", log->PID);
                
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%d", log->TID);
                
                if(log->package) {
                    ImGui::TableSetColumnIndex(4);
                    ImGui::Text(log->package);
                }
                
                ImGui::TableSetColumnIndex(5);
                ImGui::Text("%c", (char) log->priority);
                
                if(log->package) {
                    ImGui::TableSetColumnIndex(6);
                    ImGui::Text(log->tag);
                }
                
                ImGui::TableSetColumnIndex(7);
                ImGui::TextWrapped(log->message);
            }
            ImGui::EndTable();
        }
        ImGui::End();
        
        
        
        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
        /*{
            static float f = 0.0f;
            static int counter = 0;
            
            ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.
            
            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_another_window);
            
            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color
            
            if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);
            
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
        }*/
        
        
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
