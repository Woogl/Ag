:: Copyright Woogle. All Rights Reserved.
@echo off
chcp 65001 >nul
setlocal

rem 에디터는 C++ 소스가 바뀌어도 남아 있는 예전 DLL로 경고 없이 열리므로, 열기 전에 항상 빌드한다.
set "PROJECT=%~dp0Ag.uproject"
set "ENGINE="

for /f "usebackq delims=" %%D in (`powershell -NoProfile -Command "$a = (Get-Content -Raw -LiteralPath $env:PROJECT | ConvertFrom-Json).EngineAssociation; (Get-ItemProperty ('HKLM:\SOFTWARE\EpicGames\Unreal Engine\' + $a) -ErrorAction SilentlyContinue).InstalledDirectory"`) do set "ENGINE=%%D"
if defined ENGINE if not exist "%ENGINE%\Engine\Build\BatchFiles\Build.bat" set "ENGINE="

rem 엔진 업데이트 뒤 레지스트리 등록이 없으면 런처 설치 기록에서 찾는다.
if not defined ENGINE (
    for /f "usebackq delims=" %%D in (`powershell -NoProfile -Command "$a = (Get-Content -Raw -LiteralPath $env:PROJECT | ConvertFrom-Json).EngineAssociation; $p = Join-Path $env:ProgramData 'Epic\UnrealEngineLauncher\LauncherInstalled.dat'; if (Test-Path -LiteralPath $p) { $items = (Get-Content -Raw -LiteralPath $p | ConvertFrom-Json).InstallationList; foreach ($item in $items) { if ($item.AppName -eq ('UE_' + $a)) { $item.InstallLocation; break } } }"`) do set "ENGINE=%%D"
)
if defined ENGINE if not exist "%ENGINE%\Engine\Build\BatchFiles\Build.bat" set "ENGINE="
if defined ENGINE if not exist "%ENGINE%\Engine\Binaries\Win64\UnrealEditor.exe" set "ENGINE="
if not defined ENGINE (
    echo Ag.uproject의 엔진 버전에 맞는 언리얼 엔진 설치를 찾지 못했습니다.
    pause
    exit /b 1
)

rem 기획자가 새 C++ 파일을 받고 솔루션 재생성을 잊어도 되도록, 소스 파일 목록·빌드 설정·엔진이 지난 재생성 뒤 바뀌었으면 재생성한다.
set "STAMP=%~dp0Intermediate\ProjectFiles\OpenEditor.stamp"
set "FINGERPRINT="
set "OLDPRINT="
for /f "usebackq delims=" %%H in (`powershell -NoProfile -Command "$root = Split-Path -LiteralPath $env:PROJECT; $lines = @($env:ENGINE, (Get-FileHash -LiteralPath $env:PROJECT).Hash); foreach ($dir in 'Source', 'Plugins') { $p = Join-Path $root $dir; if (Test-Path -LiteralPath $p) { Get-ChildItem -LiteralPath $p -Recurse -File | Where-Object { $_.FullName -notmatch '\\(Binaries|Intermediate)\\' } | Sort-Object FullName | ForEach-Object { $lines += $_.FullName.Substring($root.Length); if ($_.Extension -in '.cs', '.uplugin') { $lines += (Get-FileHash -LiteralPath $_.FullName).Hash } } } }; $bytes = [Text.Encoding]::UTF8.GetBytes($lines -join [char]10); [BitConverter]::ToString([Security.Cryptography.SHA256]::Create().ComputeHash($bytes)).Replace('-', '')"`) do set "FINGERPRINT=%%H"
if exist "%STAMP%" set /p OLDPRINT=<"%STAMP%"
if not exist "%~dp0Intermediate\ProjectFiles\Ag.vcxproj" set "OLDPRINT="
if defined FINGERPRINT if "%FINGERPRINT%"=="%OLDPRINT%" goto Build

echo 솔루션 파일을 다시 만듭니다.
call "%ENGINE%\Engine\Build\BatchFiles\Build.bat" -ProjectFiles "-Project=%PROJECT%" -Game -Rocket
if errorlevel 1 (
    echo.
    echo 솔루션 파일을 만들지 못했습니다. 이 창의 내용을 프로그래머에게 보내 주세요.
    pause
    exit /b 1
)
if defined FINGERPRINT >"%STAMP%" echo %FINGERPRINT%

:Build
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
