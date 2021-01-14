@echo off

set exe_name=Catlog
set compile_flags= -nologo /Zi /FC /I ../include/
set linker_flags= glfw3dll.lib gdi32.lib user32.lib kernel32.lib opengl32.lib
set linker_path="../lib/"

if not exist build mkdir build
pushd build
start /b /wait "" "cl.exe" %compile_flags% ../src/main.cpp /link %linker_flags% /libpath:%linker_path% /out:%exe_name%.exe
copy ..\lib\* . >NUL
copy ..\resources\* . >NUL
popd
