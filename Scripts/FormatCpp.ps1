# Copyright Woogle. All Rights Reserved.
[CmdletBinding()]
param(
    [switch]$Write,
    [string[]]$Files,
    [string]$ClangFormat
)

$ErrorActionPreference = 'Stop'
$RequiredVersion = '22.1.3'
$ProjectRoot = Split-Path $PSScriptRoot -Parent

try {
    if (-not $ClangFormat) {
        $Command = Get-Command clang-format -ErrorAction SilentlyContinue
        if ($Command) {
            $ClangFormat = $Command.Source
        } else {
            $VsWhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
            if (Test-Path -LiteralPath $VsWhere) {
                $ClangFormat = & $VsWhere -latest -products '*' -find 'VC/Tools/Llvm/x64/bin/clang-format.exe' | Select-Object -First 1
            }
        }
    }
    if (-not $ClangFormat) {
        throw "clang-format $RequiredVersion is required. Supply -ClangFormat <exe path>."
    }
    $VersionText = & $ClangFormat --version
    if ($LASTEXITCODE -ne 0 -or "$VersionText" -notmatch "\bversion $([regex]::Escape($RequiredVersion))(?:\s|$)") {
        throw "Expected clang-format $RequiredVersion; got: $VersionText"
    }

    $Roots = @('Source/Ag', 'Source/AgEditor') | ForEach-Object {
        [IO.Path]::GetFullPath((Join-Path $ProjectRoot $_)) + [IO.Path]::DirectorySeparatorChar
    }
    $Targets = if ($Files) {
        foreach ($File in $Files) {
            $Candidate = if ([IO.Path]::IsPathRooted($File)) { $File } else { Join-Path $ProjectRoot $File }
            (Get-Item -LiteralPath $Candidate).FullName
        }
    } else {
        Get-ChildItem -LiteralPath $Roots -Recurse -File | Where-Object { $_.Extension -in '.h', '.cpp' } | Select-Object -ExpandProperty FullName
    }
    $Targets = @($Targets | Sort-Object -Unique)
    if ($Targets.Count -eq 0) { throw 'No C++ files selected.' }
    # Validate the complete selection before modifying any file.
    foreach ($Target in $Targets) {
        $InScope = @($Roots | Where-Object { $Target.StartsWith($_, [StringComparison]::OrdinalIgnoreCase) }).Count -gt 0
        if (-not $InScope -or [IO.Path]::GetExtension($Target) -notin '.h', '.cpp') {
            throw "Outside formatter scope: $Target"
        }
    }

    $Arguments = @("--style=file:$ProjectRoot/.clang-format", '--fallback-style=none', '--Werror')
    if ($Write) { $Arguments += '-i' } else { $Arguments += @('--dry-run', '--Werror') }
    $Failed = 0
    foreach ($Target in $Targets) {
        & $ClangFormat @Arguments $Target
        if ($LASTEXITCODE -ne 0) { $Failed++ }
    }
    Write-Host "Files: $($Targets.Count); failures: $Failed; write: $Write"
    if ($Failed -gt 0) { exit 1 }
    exit 0
} catch {
    Write-Error $_ -ErrorAction Continue
    exit 2
}
