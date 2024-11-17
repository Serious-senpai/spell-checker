@echo off
setlocal enabledelayedexpansion

g++ --version

for %%f in ("%~dp0..") do set root=%%~ff
echo Got root of repository: %root%
if not exist %root%\build mkdir %root%\build

echo Building main.exe
set params=-Wall -I %root%\src\include -D LOGGING -std=c++20
if "%1"=="debug" (
    set params=!params! -g -D DEBUG
    echo Building in debug mode
) else (
    set params=!params! -O3
    echo Building normally
)

set command=g++ %params% %root%\src\main.cpp -o %root%\build\main.exe
echo Running "%command%"
%command%
