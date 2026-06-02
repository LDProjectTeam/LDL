@echo off
chcp 65001 >nul
color 0A
title LDLauncher Build and Test Tool

cd /d "%~dp0"

set "PATH=%PATH%;%PROGRAMFILES%\nodejs;%PROGRAMFILES(X86)%\nodejs;%LOCALAPPDATA%\Programs\nodejs;%APPDATA%\npm"

where npm >nul 2>&1
if %errorlevel% neq 0 (
    echo.
    echo  [ERROR] npm not found! Install Node.js from https://nodejs.org/
    echo.
    pause
    exit /b 1
)

:MENU
cls
echo.
echo  ==========================================
echo       LDLauncher  --  Build and Test
echo  ==========================================
echo.
echo   [1]  Test in Browser          (Vite Dev Server)
echo   [2]  Test as Desktop App      (build + Electron)
echo.
echo   [3]  Build Installer          (Ask Version)
echo   [4]  Build Portable           (Ask Version)
echo   [5]  Build Installer + Port.  (Ask Version)
echo.
echo   [6]  Build Portable + Run it
echo   [7]  Build Installer + Run it
echo.
echo   [8]  ⭐ RELEASE NEW UPDATE      (Bump Version and Build)
echo.
echo   [9]  Only build React         (npm run build)
echo   [L]  Lint / Code check
echo.
echo   [0]  Exit
echo.
set /p "choice=  Choose option: "

if "%choice%"=="1" goto DEV_BROWSER
if "%choice%"=="2" goto DEV_ELECTRON
if "%choice%"=="3" goto BUILD_INSTALLER
if "%choice%"=="4" goto BUILD_PORTABLE
if "%choice%"=="5" goto BUILD_BOTH
if "%choice%"=="6" goto BUILD_RUN_PORTABLE
if "%choice%"=="7" goto BUILD_RUN_INSTALLER
if "%choice%"=="8" goto RELEASE_UPDATE
if "%choice%"=="9" goto BUILD_REACT
if /I "%choice%"=="L" goto LINT
if "%choice%"=="0" goto EXIT

echo.
echo  [!] Invalid choice. Try again.
timeout /t 2 >nul
goto MENU

:: -------------------------------------------------
:GET_CURRENT_VER
for /f "delims=" %%V in ('node -e "process.stdout.write(require('./package.json').version)"') do set "CURRENT_VER=%%V"
goto :eof

:: -------------------------------------------------
:ASK_VERSION
call :GET_CURRENT_VER
echo.
echo  ----------------------------------------
echo   Current version: %CURRENT_VER%
echo  ----------------------------------------
echo.
set /p "NEW_VER=  New version (Enter = keep %CURRENT_VER%): "
if "%NEW_VER%"=="" set "NEW_VER=%CURRENT_VER%"

echo %NEW_VER%| findstr /r "^[0-9]" >nul
if %errorlevel% neq 0 goto :VER_INVALID
node -e "process.exit(/^\d+\.\d+\.\d+$/.test('%NEW_VER%')?0:1)" >nul 2>&1
if %errorlevel% neq 0 goto :VER_INVALID
goto :VER_OK
:VER_INVALID
    echo.
    echo  [!] Invalid format. Use X.Y.Z  (e.g. 3.1.0)
    set "NEW_VER="
    goto ASK_VERSION
:VER_OK

if not "%NEW_VER%"=="%CURRENT_VER%" (
    node -e "const fs=require('fs'),p=JSON.parse(fs.readFileSync('package.json','utf8'));p.version='%NEW_VER%';fs.writeFileSync('package.json',JSON.stringify(p,null,2)+'\n');"
    echo  [OK] Version updated: %CURRENT_VER% --^> %NEW_VER%
) else (
    echo  [OK] Version unchanged: %NEW_VER%
)
echo.
goto :eof

:: -------------------------------------------------
:DEV_BROWSER
cls
echo.
echo  [>>] Starting Vite Dev Server...
echo.
call npm run dev
echo.
echo  [i] Dev server stopped.
pause
goto MENU

:: -------------------------------------------------
:DEV_ELECTRON
cls
echo.
echo  [>>] Building React...
call npm run build
if %errorlevel% neq 0 (
    echo  [ERROR] React build failed. Fix errors above.
    pause
    goto MENU
)
echo.
echo  [>>] Launching Electron...
call npm start
if %errorlevel% neq 0 (
    echo  [ERROR] Electron failed to start.
)
pause
goto MENU

:: -------------------------------------------------
:BUILD_REACT
cls
echo.
echo  [>>] Building React (tsc + vite build)...
call npm run build
if %errorlevel% neq 0 (
    echo  [ERROR] Build failed.
    pause
    goto MENU
)
echo.
echo  [OK] React build successful.
pause
goto MENU

:: -------------------------------------------------
:: Clean previous build files prompt
:ASK_CLEAN_DIR
echo.
set /p "CLEAN_DIR=  Delete ALL previous files in build folder? (Y/N): "
if /I "%CLEAN_DIR%"=="Y" (
    echo  [>>] Cleaning build folder...
    if exist "..\LDLauncher_Test_Build" rd /s /q "..\LDLauncher_Test_Build"
    mkdir "..\LDLauncher_Test_Build"
)
goto :eof

:: -------------------------------------------------
:: Clean electron-builder junk files
:CLEANUP_JUNK
echo  [>>] Cleaning up temporary build files...
if exist "..\LDLauncher_Test_Build\win-unpacked" rd /s /q "..\LDLauncher_Test_Build\win-unpacked"
if exist "..\LDLauncher_Test_Build\.icon-ico" rd /s /q "..\LDLauncher_Test_Build\.icon-ico"
if exist "..\LDLauncher_Test_Build\linux-unpacked" rd /s /q "..\LDLauncher_Test_Build\linux-unpacked"
if exist "..\LDLauncher_Test_Build\mac" rd /s /q "..\LDLauncher_Test_Build\mac"
if exist "..\LDLauncher_Test_Build\builder-debug.yml" del /f /q "..\LDLauncher_Test_Build\builder-debug.yml"
if exist "..\LDLauncher_Test_Build\builder-effective-config.yaml" del /f /q "..\LDLauncher_Test_Build\builder-effective-config.yaml"
if exist "..\LDLauncher_Test_Build\*.blockmap" del /f /q "..\LDLauncher_Test_Build\*.blockmap"
goto :eof

:: -------------------------------------------------
:BUILD_INSTALLER
cls
call :ASK_VERSION
call :ASK_CLEAN_DIR
echo  [>>] Building React...
call npm run build
if %errorlevel% neq 0 (
    echo  [ERROR] React build failed.
    pause
    goto MENU
)
echo.
echo  [>>] Packaging Installer (NSIS) v%NEW_VER%...
call npx electron-builder --win nsis
if %errorlevel% neq 0 (
    echo  [ERROR] electron-builder failed.
    pause
    goto MENU
)
call :CLEANUP_JUNK
echo.
echo  [OK] LDLauncher_Setup_%NEW_VER%.exe ready in: ..\LDLauncher_Test_Build\
pause
goto MENU

:: -------------------------------------------------
:BUILD_PORTABLE
cls
call :ASK_VERSION
call :ASK_CLEAN_DIR
echo  [>>] Building React...
call npm run build
if %errorlevel% neq 0 (
    echo  [ERROR] React build failed.
    pause
    goto MENU
)
echo.
echo  [>>] Packaging Portable v%NEW_VER%...
call npx electron-builder --win portable
if %errorlevel% neq 0 (
    echo  [ERROR] electron-builder failed.
    pause
    goto MENU
)
call :CLEANUP_JUNK
echo.
echo  [OK] LDLauncher_%NEW_VER%.exe ready in: ..\LDLauncher_Test_Build\
pause
goto MENU

:: -------------------------------------------------
:BUILD_BOTH
cls
call :ASK_VERSION
call :ASK_CLEAN_DIR
echo  [>>] Building React...
call npm run build
if %errorlevel% neq 0 (
    echo  [ERROR] React build failed.
    pause
    goto MENU
)
echo.
echo  [>>] Packaging Installer + Portable v%NEW_VER%...
call npx electron-builder --win nsis portable
if %errorlevel% neq 0 (
    echo  [ERROR] electron-builder failed.
    pause
    goto MENU
)
call :CLEANUP_JUNK
echo.
echo  [OK] LDLauncher_Setup_%NEW_VER%.exe + LDLauncher_%NEW_VER%.exe ready in: ..\LDLauncher_Test_Build\
pause
goto MENU

:: -------------------------------------------------
:BUILD_RUN_PORTABLE
cls
call :ASK_VERSION
call :ASK_CLEAN_DIR
echo  [>>] Building React...
call npm run build
if %errorlevel% neq 0 (
    echo  [ERROR] React build failed.
    pause
    goto MENU
)
echo.
echo  [>>] Packaging Portable v%NEW_VER%...
call npx electron-builder --win portable
if %errorlevel% neq 0 (
    echo  [ERROR] electron-builder failed.
    pause
    goto MENU
)
call :CLEANUP_JUNK
echo.
echo  [>>] Searching for Portable exe...
set "FOUND_EXE="
for /f "delims=" %%F in ('dir /b /s "..\LDLauncher_Test_Build\LDLauncher_%NEW_VER%.exe" 2^>nul') do set "FOUND_EXE=%%F"
if not defined FOUND_EXE (
    for /f "delims=" %%F in ('dir /b /s "..\LDLauncher_Test_Build\*.exe" 2^>nul') do set "FOUND_EXE=%%F"
)
if defined FOUND_EXE (
    echo  [OK] Launching: %FOUND_EXE%
    start "" "%FOUND_EXE%"
) else (
    echo  [!] Portable exe not found in ..\LDLauncher_Test_Build\
)
pause
goto MENU

:: -------------------------------------------------
:BUILD_RUN_INSTALLER
cls
call :ASK_VERSION
call :ASK_CLEAN_DIR
echo  [>>] Building React...
call npm run build
if %errorlevel% neq 0 (
    echo  [ERROR] React build failed.
    pause
    goto MENU
)
echo.
echo  [>>] Packaging Installer (NSIS) v%NEW_VER%...
call npx electron-builder --win nsis
if %errorlevel% neq 0 (
    echo  [ERROR] electron-builder failed.
    pause
    goto MENU
)
call :CLEANUP_JUNK
echo.
echo  [>>] Searching for Setup exe...
set "FOUND_SETUP="
for /f "delims=" %%F in ('dir /b /s "..\LDLauncher_Test_Build\LDLauncher_Setup_%NEW_VER%.exe" 2^>nul') do set "FOUND_SETUP=%%F"
if not defined FOUND_SETUP (
    for /f "delims=" %%F in ('dir /b /s "..\LDLauncher_Test_Build\*Setup*.exe" 2^>nul') do set "FOUND_SETUP=%%F"
)
if defined FOUND_SETUP (
    echo  [OK] Launching installer: %FOUND_SETUP%
    start "" "%FOUND_SETUP%"
) else (
    echo  [!] Setup exe not found in ..\LDLauncher_Test_Build\
)
pause
goto MENU

:: -------------------------------------------------
:RELEASE_UPDATE
cls
echo.
echo  [>>] STARTING RELEASE PROCESS...
call :ASK_VERSION
call :ASK_CLEAN_DIR
echo  [>>] Building React...
call npm run build
if %errorlevel% neq 0 (
    echo  [ERROR] React build failed.
    pause
    goto MENU
)
echo.
echo  [>>] Packaging Installer (NSIS) v%NEW_VER%...
call npx electron-builder --win nsis
if %errorlevel% neq 0 (
    echo  [ERROR] electron-builder failed.
    pause
    goto MENU
)
call :CLEANUP_JUNK
echo.
echo  [OK] RELEASE READY! 
echo       File: ..\LDLauncher_Test_Build\LDLauncher_Setup_%NEW_VER%.exe
echo       Next step: Upload this file to GitHub Releases as version %NEW_VER%
pause
goto MENU

:: -------------------------------------------------
:LINT
cls
echo.
echo  [>>] Running ESLint...
call npm run lint
echo.
pause
goto MENU

:: -------------------------------------------------
:EXIT
cls
echo.
echo  Goodbye!
echo.
exit /b 0
