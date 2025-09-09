@echo off
echo Building UI Demo...

REM Set the paths
set SOURCE=ui_demo.cpp src\input.cpp
set OUTPUT=output\ui_demo.exe
set INCLUDE_PATHS=-I. -Iinclude -IC:/msys64/ucrt64/include/SDL2
set LIBRARY_PATHS=-LC:/msys64/ucrt64/lib
set LIBRARIES=-lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -lopengl32 -lglu32

REM Create the output directory if it doesn't exist
if not exist output mkdir output

REM Compile the program
g++ -std=c++17 %SOURCE% %INCLUDE_PATHS% %LIBRARY_PATHS% %LIBRARIES% -o %OUTPUT%

REM Check if compilation was successful
if %ERRORLEVEL% == 0 (
    echo Build successful!
    echo Running UI Demo...
    %OUTPUT%
) else (
    echo Build failed!
)

pause
