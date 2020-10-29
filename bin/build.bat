@echo off

set compiler_flags=-Od -MTd -nologo -fp:fast -fp:except- -Gm- -GR- -EHa- -Zo -Oi -WX -W4 -wd4624 -wd4530 -wd4201 -wd4100 -wd4101 -wd4189 -wd4505 -wd4127 -wd4702 -wd4310 -FC -Z7 -I%PROJECT_PATH%/src
set linker_flags= -incremental:no -opt:ref user32.lib gdi32.lib winmm.lib Shlwapi.lib Ws2_32.lib

IF NOT exist %BUILD_PATH% ( mkdir %BUILD_PATH% )
pushd %BUILD_PATH%

cl %compiler_flags% %PROJECT_PATH%\src\test.cpp -Feparser_combinator_test.exe /link %linker_flags%
cl %compiler_flags% %PROJECT_PATH%\examples\lisp.cpp -Feparser_combinator_lisp.exe /link %linker_flags%
cl %compiler_flags% %PROJECT_PATH%\examples\bit.cpp -Feparser_combinator_bit.exe /link %linker_flags%

popd
