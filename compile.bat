::x86_64-w64-mingw32-windres version.rc -O coff -o version.res
::x86_64-w64-mingw32-gcc -O2 hotcorner.c version.res -o hotcorner.exe -Wl,-subsystem,windows

::version info
"D:\Ferramentas\mingw64\bin\windres.exe" version.rc -O coff -o .\build\version.res

::compilation
"D:\Ferramentas\mingw64\bin\x86_64-w64-mingw32-gcc.exe" -O2 hotcorner.c .\build\version.res -o .\build\volumouse.exe -Wl,-subsystem,windows
