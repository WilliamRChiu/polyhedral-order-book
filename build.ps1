# Build wrapper: initializes the MSVC x64 dev env in this PowerShell session
# and runs the standard configure/build/test cycle. Run from the repo root.
#
# Usage:
#   .\build.ps1            # configure (if needed), build, run tests
#   .\build.ps1 -NoTest    # skip ctest
#   .\build.ps1 -Clean     # wipe build-ninja first

param(
    [switch]$NoTest,
    [switch]$Clean
)

$ErrorActionPreference = 'Stop'

$vcvarsall = 'C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvarsall.bat'
if (-not (Test-Path $vcvarsall)) {
    throw "vcvarsall.bat not found at $vcvarsall. Install VS Build Tools 2019 or edit this script."
}

# Import the x64 dev env into the current PowerShell session by running
# vcvarsall.bat in cmd, dumping its env, and re-injecting each var.
if (-not $env:INCLUDE -or $env:LIB -notlike '*\x64*') {
    Write-Host 'Initializing MSVC x64 environment...' -ForegroundColor Cyan
    $tmp = New-TemporaryFile
    cmd /c "`"$vcvarsall`" amd64 > nul 2>&1 && set > `"$tmp`""
    Get-Content $tmp | ForEach-Object {
        if ($_ -match '^([^=]+)=(.*)$') {
            Set-Item -Path "env:$($matches[1])" -Value $matches[2]
        }
    }
    Remove-Item $tmp
}

if ($Clean -and (Test-Path build-ninja)) {
    Write-Host 'Wiping build-ninja...' -ForegroundColor Cyan
    Remove-Item -Recurse -Force build-ninja
}

if (-not (Test-Path build-ninja\build.ninja)) {
    Write-Host 'Configuring with CMake...' -ForegroundColor Cyan
    cmake -B build-ninja -G Ninja -S .
    if ($LASTEXITCODE -ne 0) { throw "cmake configure failed" }
}

Write-Host 'Building...' -ForegroundColor Cyan
cmake --build build-ninja -j
if ($LASTEXITCODE -ne 0) { throw "build failed" }

if (-not $NoTest) {
    Write-Host 'Running tests...' -ForegroundColor Cyan
    ctest --test-dir build-ninja --output-on-failure
}
