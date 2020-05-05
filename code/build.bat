@echo off 

rmdir /Q /S .\build\
mkdir .\build\
pushd .\build\
cl -Zi ..\code\win32_sudoku.c user32.lib
popd