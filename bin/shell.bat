@echo off

SET PROJECT_NAME=pc
SET PROJECT_MAIN_FILE=main.cpp
SET PROJECT_LINKER_FLAGS=user32.lib gdi32.lib winmm.lib Shlwapi.lib

SET PATH=%userprofile%\Dev\Cpp\bin;%userprofile%\Dev\Cpp\bin;%PATH%

call global_shell.bat
