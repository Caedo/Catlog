@echo off

if NOT "%Platform%" == "X64" IF NOT "%Platform%" == "x64" (call vcvarsall x64)

set exe_name=Catlog
set compile_flags= -nologo /Zi /FC /I ../include/ /W4
set linker_flags= glfw3dll.lib gdi32.lib user32.lib kernel32.lib opengl32.lib Comdlg32.lib
set linker_path="../lib/"

if not exist build mkdir build
pushd build
REM start /b /wait "" "cl.exe" -DMTR_ENABLED %compile_flags% ../src/stub.cpp /link %linker_flags% /libpath:%linker_path% /out:%exe_name%.exe
start /b /wait "" "cl.exe" %compile_flags% ../src/stub.cpp /link %linker_flags% /libpath:%linker_path% /out:%exe_name%.exe
copy ..\lib\* . >NUL
copy ..\resources\* . >NUL
popd
