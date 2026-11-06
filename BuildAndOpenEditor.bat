:: Copyright Woogle. All Rights Reserved.
@echo off
chcp 65001 >nul
setlocal

rem 에디터는 C++ 소스가 바뀌어도 남아 있는 예전 DLL로 경고 없이 열리므로, 열기 전에 항상 빌드한다.
set "PROJECT=%~dp0Ag.uproject"

for /f "usebackq delims=" %%D in (`powershell -NoProfile -Command "$a = (Get-Content -Raw $env:PROJECT | ConvertFrom-Json).EngineAssociation; (Get-ItemProperty ('HKLM:\SOFTWARE\EpicGames\Unreal Engine\' + $a) -ErrorAction SilentlyContinue).InstalledDirectory"`) do set "ENGINE=%%D"
if not defined ENGINE (
    echo Ag.uproject의 엔진 버전에 맞는 언리얼 엔진 설치를 찾지 못했습니다.
    pause
    exit /b 1
)

echo 빌드합니다. 바뀐 코드가 없으면 금방 끝납니다.
call "%ENGINE%\Engine\Build\BatchFiles\Build.bat" AgEditor Win64 DebugGame "-Project=%PROJECT%" -WaitMutex
if errorlevel 1 (
    echo.
    echo 빌드에 실패했습니다. 에디터가 켜져 있으면 닫고 다시 실행하세요.
    echo 그래도 실패하면 이 창의 내용을 프로그래머에게 보내 주세요.
    pause
    exit /b 1
)

start "" "%ENGINE%\Engine\Binaries\Win64\UnrealEditor.exe" "%PROJECT%" -debug
