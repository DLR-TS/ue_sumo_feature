<#
.LICENSE
Copyright (c) 2026 DLR e.V.

This program and the accompanying materials are made
available under the terms of the Eclipse Public License 2.0
which is available at https://www.eclipse.org/legal/epl-2.0/

SPDX-License-Identifier: EPL-2.0

.SYNOPSIS
Builds all GEMSTAR third-party dependencies natively on Windows.

.DESCRIPTION
Runs the PowerShell dependency build script of every plugin.
#>
param(
    [string]$SumoRef = $env:SUMO_REF,
    [string]$SourceDir = $env:SUMO_SOURCE_DIR,
    [string]$BuildDir = $env:SUMO_BUILD_DIR,
    [string]$Generator = $env:CMAKE_GENERATOR,
    [string]$Configuration = "Release",
    [string]$VcpkgRoot = $env:VCPKG_ROOT,
    [string]$VcpkgTriplet = $env:VCPKG_DEFAULT_TRIPLET
)

$ErrorActionPreference = "Stop"

function Invoke-Checked {
    param(
        [string]$FilePath,
        [string[]]$Arguments
    )

    & $FilePath @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$FilePath failed with exit code $LASTEXITCODE."
    }
}

if ([string]::IsNullOrWhiteSpace($SumoRef)) {
    $SumoRef = "v1_27_1"
}

if ([string]::IsNullOrWhiteSpace($VcpkgRoot)) {
    $VcpkgRoot = "C:\gemstar-vcpkg"
}

if ([string]::IsNullOrWhiteSpace($VcpkgTriplet)) {
    $VcpkgTriplet = "x64-windows"
}

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RootDir = Resolve-Path (Join-Path $ScriptDir "..")
$ThirdPartyDir = Join-Path $RootDir "Source\ue_sumo_featureRuntime\ThirdParty\sumo"
$IncludeDir = Join-Path $ThirdPartyDir "Include"
$LibDir = Join-Path $ThirdPartyDir "Lib"
$WorkDir = Join-Path $RootDir ".sumo-build"

if ([string]::IsNullOrWhiteSpace($SourceDir)) {
    $SourceDir = "C:\gemstar-sumo-src"
}

if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    $BuildDir = "C:\gemstar-sumo-build"
}

New-Item -ItemType Directory -Force -Path $IncludeDir, $LibDir, $WorkDir | Out-Null

if (!(Test-Path $SourceDir)) {
    Invoke-Checked "git" @("-c", "core.longpaths=true", "clone", "--branch", $SumoRef, "--depth", "1", "https://github.com/eclipse-sumo/sumo.git", $SourceDir)
} else {
    Write-Host "Using existing SUMO source directory: $SourceDir"
}

Invoke-Checked "git" @("-C", $SourceDir, "fetch", "--tags", "--force")
Invoke-Checked "git" @("-C", $SourceDir, "checkout", "--force", $SumoRef)

$VcpkgToolchain = Join-Path $VcpkgRoot "scripts\buildsystems\vcpkg.cmake"
$VcpkgExe = Join-Path $VcpkgRoot "vcpkg.exe"

if (!(Test-Path $VcpkgRoot)) {
    Invoke-Checked "git" @("clone", "https://github.com/microsoft/vcpkg.git", $VcpkgRoot)
}

if (!(Test-Path $VcpkgExe)) {
    $BootstrapScript = Join-Path $VcpkgRoot "bootstrap-vcpkg.bat"
    if (!(Test-Path $BootstrapScript)) {
        throw "Could not find vcpkg bootstrap script at $BootstrapScript."
    }
    Invoke-Checked $BootstrapScript @()
}

if (!(Test-Path $VcpkgToolchain)) {
    throw "Could not find vcpkg.cmake at $VcpkgToolchain."
}

Invoke-Checked $VcpkgExe @("install", "xerces-c:$VcpkgTriplet", "zlib:$VcpkgTriplet")

$SwigVersion = "4.4.1"
$SwigRoot = Join-Path $WorkDir "swigwin-$SwigVersion"
$SwigExe = Join-Path $SwigRoot "swig.exe"

if (!(Test-Path $SwigExe)) {
    $SwigZip = Join-Path $WorkDir "swigwin-$SwigVersion.zip"

    if (Test-Path $SwigZip) {
        Remove-Item $SwigZip -Force
    }

    $SwigUrl = "https://downloads.sourceforge.net/project/swig/swigwin/swigwin-$SwigVersion/swigwin-$SwigVersion.zip"

    Write-Host "Downloading SWIG $SwigVersion..."

	Invoke-Checked "curl.exe" @(
		"--location",
		"--fail",
		"--show-error",
		"--silent",
		"--output", $SwigZip,
		$SwigUrl
	)

    if (!(Test-Path $SwigZip)) {
        throw "SWIG archive was not downloaded."
    }

    Expand-Archive `
        -Path $SwigZip `
        -DestinationPath $WorkDir `
        -Force

    Remove-Item $SwigZip -Force
}

if (!(Test-Path $SwigExe)) {
    throw "Could not find SWIG at $SwigExe."
}

Write-Host "Using SWIG: $SwigExe"
Invoke-Checked $SwigExe @("-version")

if (Test-Path $BuildDir) {
    Remove-Item -Recurse -Force $BuildDir
}

New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null

$CMakeArgs = @(
    "-S", $SourceDir,
    "-B", $BuildDir,
    "-A", "x64",
    "-DENABLE_PYTHON_BINDINGS=OFF",
    "-DENABLE_JAVA_BINDINGS=OFF",
    "-DENABLE_CS_BINDINGS=OFF",
    "-DENABLE_C_BINDINGS=ON",

    "-DENABLE_GTEST=OFF",
    "-DENABLE_FOX=OFF",
    "-DENABLE_GDAL=OFF",
    "-DENABLE_PROJ=OFF",
    "-DENABLE_PARQUET=OFF",
    "-DENABLE_EIGEN=OFF",
    "-DENABLE_JUPEDSIM=OFF",
    "-DENABLE_OSG=OFF",
    "-DENABLE_FFMPEG=OFF",
    "-DENABLE_GL2PS=OFF",

    "-DCHECK_OPTIONAL_LIBS=OFF"
)

if (![string]::IsNullOrWhiteSpace($Generator)) {
    $CMakeArgs += @("-G", $Generator)
}

$SwigExeCMake = [string]$SwigExe
$SwigExeCMake = $SwigExeCMake.Replace('\', '/')
$SwigDirCMake = (Join-Path $SwigRoot "Lib").Replace('\', '/')

if (!(Test-Path (Join-Path $SwigRoot "Lib\swig.swg"))) {
    throw "Could not find the SWIG library file: $SwigRoot\Lib\swig.swg"
}

$CMakeArgs += @(
    "-DCMAKE_TOOLCHAIN_FILE=$VcpkgToolchain",
    "-DVCPKG_TARGET_TRIPLET=$VcpkgTriplet",
    "-DSWIG_EXECUTABLE:FILEPATH=$SwigExeCMake",
    "-DSWIG_DIR:PATH=$SwigDirCMake"
)

Write-Host "CMake SWIG executable: $SwigExeCMake"
Write-Host "CMake SWIG library:    $SwigDirCMake"

Invoke-Checked "cmake" $CMakeArgs
Invoke-Checked "cmake" @("--build", $BuildDir, "--config", $Configuration, "--target", "libsumoc", "--parallel")

$SumoBinDir = Join-Path $SourceDir "bin"

$LibsumocLibPath = Join-Path $SumoBinDir "libsumoc.lib"
$LibsumocDllPath = Join-Path $SumoBinDir "libsumoc.dll"

if (!(Test-Path $LibsumocLibPath)) {
    $LibsumocLib = Get-ChildItem `
        -Path $BuildDir, $SourceDir `
        -Recurse `
        -Filter "libsumoc.lib" `
        -ErrorAction SilentlyContinue |
        Select-Object -First 1
} else {
    $LibsumocLib = Get-Item $LibsumocLibPath
}

if (!(Test-Path $LibsumocDllPath)) {
    $LibsumocDll = Get-ChildItem `
        -Path $BuildDir, $SourceDir `
        -Recurse `
        -Filter "libsumoc.dll" `
        -ErrorAction SilentlyContinue |
        Select-Object -First 1
} else {
    $LibsumocDll = Get-Item $LibsumocDllPath
}

if ($null -eq $LibsumocLib) {
    throw "Could not find libsumoc.lib."
}
Write-Host "Found libsumoc library: $($LibsumocLib.FullName)"

if ($null -eq $LibsumocDll) {
    throw "Could not find libsumoc.dll."
}
Write-Host "Found libsumoc DLL:     $($LibsumocDll.FullName)"

$LibsumoIncludeSource = Join-Path $SourceDir "src\libsumo"
$LibsumoIncludeTarget = Join-Path $IncludeDir "libsumo"
if (Test-Path $LibsumoIncludeTarget) {
    Remove-Item -Recurse -Force $LibsumoIncludeTarget
}
Copy-Item -Recurse $LibsumoIncludeSource $LibsumoIncludeTarget
Get-ChildItem -Path $LibsumoIncludeTarget -File -Recurse | Where-Object { $_.Extension -ne ".h" } | Remove-Item -Force

$CWrapHeader = Join-Path $BuildDir "src\libsumo\c\libsumoC_wrap.h"
if (!(Test-Path $CWrapHeader)) {
    $CWrapHeader = (Get-ChildItem -Path $BuildDir -Recurse -Filter "libsumoC_wrap.h" | Select-Object -First 1).FullName
}

$CIncludeTarget = Join-Path $LibsumoIncludeTarget "c"
New-Item -ItemType Directory -Force -Path $CIncludeTarget | Out-Null
Copy-Item $CWrapHeader (Join-Path $CIncludeTarget "libsumoC_wrap.h") -Force

$VersionHeader = Join-Path $BuildDir "src\version.h"
if (Test-Path $VersionHeader) {
    Copy-Item $VersionHeader (Join-Path $IncludeDir "version.h") -Force
}

Remove-Item -Path (Join-Path $LibDir "*.dll"), (Join-Path $LibDir "*.lib"), (Join-Path $LibDir "*.exp"), (Join-Path $LibDir "*.pdb") -Force -ErrorAction SilentlyContinue

Copy-Item $LibsumocLib.FullName (Join-Path $LibDir "libsumoc.lib") -Force

Get-ChildItem -Path $LibsumocDll.DirectoryName -Filter "*.dll" |
    ForEach-Object { Copy-Item $_.FullName (Join-Path $LibDir $_.Name) -Force }

Write-Host "Built SUMO $SumoRef Windows C binding artefacts into $ThirdPartyDir."
