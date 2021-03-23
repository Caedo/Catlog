
#ifndef PREPROC_GEN

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
#endif //PREPROC_GEN

#include "types.h"
#include "CLArray.h"

#include "platform_win32.cpp"
#include "parser.cpp"

#include "settings.cpp"

#include "main.cpp"