@echo off
REM Old UI Demo Compilation Script

REM Create assets/fonts directory if it doesn't exist
mkdir assets\fonts 2>nul

REM Check if OpenSans-Regular.ttf exists, if not, inform the user
if not exist "assets\fonts\OpenSans-Regular.ttf" (
    echo Font file not found: assets\fonts\OpenSans-Regular.ttf
    echo Please download OpenSans-Regular.ttf and place it in the assets\fonts directory
    echo You can download it from: https://fonts.google.com/specimen/Open+Sans
    pause
    exit /b 1
)

REM Compile the UI demo
echo Compiling Old UI demo...
g++ -std=c++17 -Wall -Wextra old_ui_demo.cpp src/input.cpp -o output/old_ui_demo.exe ^
    -I. ^
    -Iinclude ^
    -IC:/msys64/ucrt64/include/SDL2 ^
    -LC:/msys64/ucrt64/lib ^
    -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf

if %ERRORLEVEL% NEQ 0 (
    echo Compilation failed
    pause
    exit /b 1
)

echo Compilation successful!
echo Running Old UI demo...

REM Run the demo
copy assets\fonts\OpenSans-Regular.ttf output\assets\fonts\OpenSans-Regular.ttf 2>nul
mkdir output\assets\fonts 2>nul
copy assets\fonts\OpenSans-Regular.ttf output\assets\fonts\OpenSans-Regular.ttf 2>nul
cd output
old_ui_demo.exe
cd ..

echo Done.
pause
