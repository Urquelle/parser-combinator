@echo off

set compiler_flags=-Od -DTEMPL_DEBUG -MTd -nologo -fp:fast -fp:except- -Gm- -GR- -EHa- -Zo -Oi -WX -W4 -wd4624 -wd4530 -wd4201 -wd4100 -wd4101 -wd4189 -wd4505 -wd4127 -wd4702 -FC -Z7 -I%PROJECT_PATH%/src
set linker_flags= -incremental:no -opt:ref user32.lib gdi32.lib winmm.lib Shlwapi.lib Ws2_32.lib
rem set linker_flags= -NODEFAULTLIB -incremental:no -opt:ref user32.lib gdi32.lib winmm.lib Shlwapi.lib Ws2_32.lib

IF NOT exist %BUILD_PATH% ( mkdir %BUILD_PATH% )
pushd %BUILD_PATH%

cl %compiler_flags% %PROJECT_PATH%\src\main.cpp -Fe%PROJECT_NAME%.exe /link %linker_flags%

popd
