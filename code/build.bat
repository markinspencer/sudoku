@echo off 

rmdir /Q /S .\build\
mkdir .\build\
pushd .\build\
cl -FAsc -Zi ..\code\win32_sudoku.c user32.lib gdi32.lib
popd