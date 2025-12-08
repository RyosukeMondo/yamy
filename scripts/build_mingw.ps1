param(
    [string]$Target = "",
    [switch]$GetBuildLog,
    [switch]$Last
)

$ErrorActionPreference = "Stop"

$MinGWPath = "C:\ProgramData\mingw64\mingw64\bin"
if (-not ($Env:PATH -like "*$MinGWPath*")) {
    $Env:PATH = "$MinGWPath;$Env:PATH"
}

# Resolve absolute paths
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$RootDir = (Resolve-Path "$ScriptDir\..").Path
$BuildDir = "$RootDir\build\mingw"
$LogDir = "$RootDir\logs"

if (-not (Test-Path $LogDir)) {
    New-Item -ItemType Directory -Path $LogDir | Out-Null
}

$LogFile = "$LogDir\build_log_MinGW.txt"

if ($GetBuildLog) {
    if (-not (Test-Path $LogFile)) {
        Write-Error "Log file not found: $LogFile"
        exit 1
    }
    
    if ($Last) {
        Get-Content $LogFile -Tail 100
    }
    else {
        Get-Content $LogFile
    }
    exit 0
}

Write-Host "Starting MinGW Build..."
Write-Host "Root Directory: $RootDir"
Write-Host "Build Directory: $BuildDir"
Write-Host "Log File: $LogFile"

if (-not (Test-Path $BuildDir)) {
    Write-Error "Build directory does not exist: $BuildDir. Please run CMake configure first."
    exit 1
}

Push-Location $BuildDir
try {
    $BuildArgs = @("--build", ".")
    if (-not [string]::IsNullOrWhiteSpace($Target)) {
        Write-Host "Target: $Target"
        $BuildArgs += "--target"
        $BuildArgs += $Target
    }
    else {
        Write-Host "Target: ALL"
    }

    # Execute cmake and capture/redirect output
    # We use cmd /c to ensure 2>&1 works as expected for native commands if powershell's redirection is quirky with cmake colors
    # But Tee-Object is usually fine.
    
    $Command = "cmake"
    $ArgsList = $BuildArgs -join " "
    
    Write-Host "Executing: $Command $ArgsList"
    
    # Run content, capturing stdout and stderr, teeing to file and host
    # Note: *>&1 combines streams
    & $Command $BuildArgs *>&1 | Tee-Object -FilePath $LogFile
}
catch {
    Write-Error "Build script failed: $_"
    exit 1
}
finally {
    Pop-Location
}
