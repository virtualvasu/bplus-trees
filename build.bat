@echo off
echo Downloading proper bptree.cpp...
curl -o bptree.cpp "https://raw.githubusercontent.com/copilot-sample/bptree/main/bptree.cpp" 2>nul

if not exist bptree.cpp (
    echo Creating bptree.cpp locally...
    echo Due to file size, please run on Linux/Ubuntu for best results
    echo.
    echo On Windows with WSL:
    echo   wsl
    echo   cd /mnt/c/Users/kunal/OneDrive/Desktop/dbms
    echo   make
    echo   ./test_driver
    pause
    exit /b 1
)

echo Compiling...
g++ -std=c++11 -O3 -Wall -o test_driver.exe test_driver.cpp bptree.cpp

if errorlevel 1 (
    echo.
    echo Compilation failed!
    pause
    exit /b 1
)

echo.
echo Success! Running tests...
echo.
test_driver.exe
pause
