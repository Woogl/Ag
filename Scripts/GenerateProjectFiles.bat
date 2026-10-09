:: Copyright Woogle. All Rights Reserved.
@echo off
setlocal

rem Invokes the same shell verb as right-clicking Ag.uproject > Generate Visual Studio project files.
rem Windows resolves UnrealVersionSelector from the registry, and it finds the engine from EngineAssociation.
for %%I in ("%~dp0..\Ag.uproject") do set "PROJECT=%%~fI"

powershell -NoProfile -Command "$ErrorActionPreference='Stop'; exit (Start-Process -FilePath $env:PROJECT -Verb rungenproj -Wait -PassThru).ExitCode"
if errorlevel 1 pause
