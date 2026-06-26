<#
.SYNOPSIS
    Configure and build OpenDeFeedback (VST3 / standalone app) on Windows.

.DESCRIPTION
    Convenience wrapper around CMake. Locates CMake (PATH or the copy bundled
    with Visual Studio), ensures the iPlug2 submodule is present, optionally
    fetches the VST3 SDK, configures the build/ tree, and builds the requested
    target(s).

.PARAMETER Target
    Which target to build: vst3, app, or all. Default: vst3.

.PARAMETER Config
    Build configuration: Release or Debug. Default: Release.

.PARAMETER Generator
    CMake generator. Default: "Visual Studio 18 2026".

.PARAMETER FetchVst3
    Download the VST3 SDK if it is missing (needed for the vst3 target).

.PARAMETER Clean
    Delete the build/ directory before configuring.

.EXAMPLE
    ./build.ps1                       # build the VST3 (Release)

.EXAMPLE
    ./build.ps1 -Target app -Config Debug

.EXAMPLE
    ./build.ps1 -Target all -FetchVst3
#>
[CmdletBinding()]
param(
    [ValidateSet('vst3', 'app', 'all')]
    [string]$Target = 'vst3',

    [ValidateSet('Release', 'Debug')]
    [string]$Config = 'Release',

    [string]$Generator = 'Visual Studio 18 2026',

    [switch]$FetchVst3,

    [switch]$Clean
)

$ErrorActionPreference = 'Stop'
$repoRoot = $PSScriptRoot
$buildDir = Join-Path $repoRoot 'build'

function Write-Step($msg) { Write-Host "==> $msg" -ForegroundColor Cyan }

# --- Locate CMake ----------------------------------------------------------
function Find-CMake {
    $cmd = Get-Command cmake -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }

    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $vsPath = & $vswhere -latest -property installationPath
        if ($vsPath) {
            $candidate = Join-Path $vsPath 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
            if (Test-Path $candidate) { return $candidate }
        }
    }
    throw "CMake not found. Install CMake or Visual Studio with the C++ CMake tools."
}

$cmake = Find-CMake
Write-Step "Using CMake: $cmake"

# --- Ensure iPlug2 submodule ----------------------------------------------
if (-not (Test-Path (Join-Path $repoRoot 'external\iPlug2\iPlug2.cmake'))) {
    Write-Step "Initializing iPlug2 submodule"
    git -C $repoRoot submodule update --init --recursive
}

# --- Optionally fetch the VST3 SDK ----------------------------------------
$vst3SdkDir = Join-Path $repoRoot 'external\iPlug2\Dependencies\IPlug\VST3_SDK'
$vst3SdkReady = Test-Path (Join-Path $vst3SdkDir 'pluginterfaces')
if (($Target -eq 'vst3' -or $Target -eq 'all') -and -not $vst3SdkReady) {
    if ($FetchVst3) {
        Write-Step "Fetching VST3 SDK (shallow clone)"
        $depsDir = Join-Path $repoRoot 'external\iPlug2\Dependencies\IPlug'
        Push-Location $depsDir
        try {
            Remove-Item -Recurse -Force VST3_SDK -ErrorAction SilentlyContinue
            git clone https://github.com/steinbergmedia/vst3sdk.git --branch master --single-branch --depth=1 VST3_SDK
            Push-Location VST3_SDK
            git submodule update --init --depth=1 pluginterfaces base public.sdk
            Pop-Location
        }
        finally { Pop-Location }
    }
    else {
        Write-Warning "VST3 SDK not found at $vst3SdkDir. The vst3 target will be skipped by CMake."
        Write-Warning "Re-run with -FetchVst3 to download it (or build -Target app)."
    }
}

# --- Clean -----------------------------------------------------------------
if ($Clean -and (Test-Path $buildDir)) {
    Write-Step "Cleaning $buildDir"
    Remove-Item -Recurse -Force $buildDir
}

# --- Configure -------------------------------------------------------------
Write-Step "Configuring ($Generator)"
& $cmake -B $buildDir -S $repoRoot -G $Generator -A x64
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed." }

# --- Build -----------------------------------------------------------------
$targets = switch ($Target) {
    'vst3' { @('OpenDeFeedback-vst3') }
    'app'  { @('OpenDeFeedback-app') }
    'all'  { @('OpenDeFeedback-vst3', 'OpenDeFeedback-app') }
}

foreach ($t in $targets) {
    Write-Step "Building $t ($Config)"
    & $cmake --build $buildDir --config $Config --target $t
    if ($LASTEXITCODE -ne 0) { throw "Build failed for $t." }
}

Write-Step "Done. Artifacts in: $buildDir\out"
