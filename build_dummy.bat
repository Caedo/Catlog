@echo off

if NOT "%Platform%" == "X64" IF NOT "%Platform%" == "x64" (call vcvarsall x64)

set exe_name=dummy_logcat
set compile_flags= -nologo /Zi /FC  /fsanitize=address
set linker_flags= user32.lib

if not exist build mkdir build
pushd build
start /b /wait "" "cl.exe" %compile_flags% ../src/dummy_logcat.cpp /link %linker_flags% /out:%exe_name%.exe
popd
