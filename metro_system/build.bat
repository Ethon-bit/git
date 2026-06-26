@echo off
echo ============================================
echo   上海地铁路径规划系统 - 编译脚本
echo ============================================
echo.

REM 尝试查找 g++
where g++ >nul 2>&1
if %ERRORLEVEL% == 0 (
    set CC=g++
    goto :compile
)

REM 尝试常见 MinGW 路径
if exist "C:\msys64\mingw64\bin\g++.exe" (
    set CC="C:\msys64\mingw64\bin\g++.exe"
    goto :compile
)
if exist "C:\MinGW\bin\g++.exe" (
    set CC="C:\MinGW\bin\g++.exe"
    goto :compile
)
if exist "D:\BaiduNetdiskDownload\MinGW\bin\g++.exe" (
    set CC="D:\BaiduNetdiskDownload\MinGW\bin\g++.exe"
    goto :compile
)

echo [错误] 未找到 g++ 编译器，请安装 MinGW 或将其加入 PATH。
pause
exit /b 1

:compile
echo [信息] 使用编译器: %CC%
echo [信息] 正在编译...
%CC% -std=c++14 -O2 -Wall -o metro.exe main.cpp
if %ERRORLEVEL% == 0 (
    echo [成功] 编译完成！可执行文件: metro.exe
    echo.
    echo 运行方式: metro.exe
) else (
    echo [失败] 编译出错，请检查错误信息。
)
pause
