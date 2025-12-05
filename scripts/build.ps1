# Modern Build Script for Yamy
# Follows "Rich" UX principles: Clear Success/Fail, Visualization, CI Awareness

param(
    [switch]$CI = $false,
    [switch]$VerboseOutput = $false,
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"
$ScriptRoot = $PSScriptRoot
$RepoRoot = Resolve-Path "$ScriptRoot\.."

# --- UX Helpers ---

# Setup CI flag based on arg or env vars
$IsCI = $CI -or ($env:CI -eq "true") -or ($env:GITHUB_ACTIONS -eq "true")
if (-not $IsCI) {
    # Check for TTY (simplified check: if Host Name is ConsoleHost)
    # In some CI envs, Host is still ConsoleHost but we rely on env vars.
}

$Colors = @{
    Green = "Green"
    Red = "Red"
    Cyan = "Cyan"
    Gray = "Gray"
    Yellow = "Yellow"
}

function Write-Log {
    param([string]$Message, [string]$Color = "Gray")
    if ($IsCI) {
        Write-Host $Message
    } else {
        Write-Host $Message -ForegroundColor $Colors[$Color]
    }
}

function Invoke-Task {
    param(
        [string]$Name,
        [scriptblock]$Action,
        [bool]$Critical = $true
    )

    $spinnerFrames = @("-", "\", "|", "/")
    
    if ($IsCI) {
        Write-Host "::group::$Name"
        try {
            & $Action
            Write-Host "::endgroup::"
            Write-Host "[OK] $Name"
        } catch {
            Write-Host "::endgroup::"
            Write-Host "[FAIL] $Name"
            Write-Host $_
            if ($Critical) { exit 1 }
        }
        return
    }

    # Interactive Mode
    Write-Host -NoNewline "   $Name"
    
    $job = Start-Job -ScriptBlock $Action
    $i = 0
    
    try {
        while ($job.State -eq 'Running') {
            $frame = $spinnerFrames[$i % 4]
            Write-Host -NoNewline "`r[$frame]"
            Start-Sleep -Milliseconds 100
            $i++
        }
    } catch {
        # Handle Ctrl+C or other interruptions
        Stop-Job $job
        Remove-Job $job
        throw
    }

    # Receive results without throwing on Job errors (since we want to print them)
    $results = Receive-Job $job -ErrorAction SilentlyContinue
    $errors = $job.ChildJobs[0].Error
    
    # Clear spinner
    Write-Host -NoNewline "`r"

    if ($job.State -eq 'Completed' -and $errors.Count -eq 0) {
        Write-Host "[✔] $Name" -ForegroundColor Green
    } else {
        Write-Host "[✘] $Name" -ForegroundColor Red
        Write-Host "---------------------------------------------------" -ForegroundColor Red
        $results | ForEach-Object { Write-Host $_ -ForegroundColor Gray }
        $errors | ForEach-Object { Write-Host $_ -ForegroundColor Red }
        Write-Host "---------------------------------------------------" -ForegroundColor Red
        
        Remove-Job $job
        if ($Critical) { exit 1 }
    }
    Remove-Job $job
}

# --- Environment Setup ---

function Get-VsDevCmd {
    # Try to find VS2022 (v17) or VS2026 (v18/Preview)
    $paths = @(
        "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat",
        "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat"
    )
    
    foreach ($p in $paths) {
        if (Test-Path $p) { return $p }
    }
    return $null
}

# --- Main Sequence ---

Write-Log "Checking Build Environment..." "Cyan"

if (-not (Get-Command msbuild -ErrorAction SilentlyContinue)) {
    Write-Log "MSBuild not found in PATH. Attempting to locate Visual Studio..." "Yellow"
    $vsDevCmd = Get-VsDevCmd
    
    if ($vsDevCmd) {
        Write-Log "Found VsDevCmd: $vsDevCmd" "Gray"
        Write-Log "Importing Environment Variables..." "Gray"
        
        # Helper to import batch environment
        $cmd = "`"$vsDevCmd`" -arch=amd64 -no_logo && set"
        cmd /c $cmd | ForEach-Object {
            if ($_ -match "^(.*?)=(.*)$") {
                Set-Content "env:\$($matches[1])" $matches[2]
            }
        }
        
        if (Get-Command msbuild -ErrorAction SilentlyContinue) {
            Write-Log "Environment initialized successfully." "Green"
        } else {
            Write-Log "Failed to initialize MSBuild even after calling VsDevCmd." "Red"
            exit 1
        }
    } else {
        Write-Log "Visual Studio Developer Command Prompt not found." "Red"
        Write-Log "Please run this script from a VS Developer PowerShell." "Red"
        exit 1
    }
}

Write-Log "Starting Yamy Build Process..." "Cyan"
Set-Location $RepoRoot

# 1. Cleanup
Invoke-Task "Cleaning previous build artifacts" {
    $ErrorActionPreference = "Stop"
    Set-Location $using:RepoRoot
    
    # Remove dirs with retry
    $dirs = @("Release", "proj\Release", "proj\x64", "proj\yamy64\x64", "proj\yamy64dll\x64", "dist")
    foreach ($d in $dirs) {
        if (Test-Path $d) { 
            try {
                Remove-Item $d -Recurse -Force -ErrorAction Stop
            } catch {
                # Fallback: Rename locked directory so we can proceed with a fresh build
                $trash = "$d.trash." + [Guid]::NewGuid().ToString().Substring(0, 8)
                try {
                    Move-Item $d $trash -Force -ErrorAction Stop
                    Write-Warning "Renamed locked directory '$d' to '$trash'. Please delete it manually after a reboot."
                } catch {
                    Write-Warning "Could not remove or rename '$d'. Please close processes using Yamy (or Explorer)."
                    throw
                }
            }
        }
    }
    
    if (-not (Test-Path "Release")) {
        New-Item -ItemType Directory -Path "Release" -Force | Out-Null
    }
}


# 2. Build x64
Invoke-Task "Building Yamy x64 ($Configuration)" {
    $ErrorActionPreference = "Stop"
    Set-Location $using:RepoRoot
    
    & msbuild proj\yamy.sln /p:Configuration=$using:Configuration /p:Platform=x64 /t:Build /v:m /nologo
    if ($LASTEXITCODE -ne 0) { throw "MSBuild failed with exit code $LASTEXITCODE" }
}

# 3. Build Win32
Invoke-Task "Building Yamy Win32 ($Configuration)" {
    $ErrorActionPreference = "Stop"
    Set-Location $using:RepoRoot
    & msbuild proj\yamy.sln /p:Configuration=$using:Configuration /p:Platform=Win32 /t:Build /v:m /nologo
    if ($LASTEXITCODE -ne 0) { throw "MSBuild failed with exit code $LASTEXITCODE" }
}

# 5. Tests
Invoke-Task "Running Tests" {
    $ErrorActionPreference = "Stop"
    Set-Location $using:RepoRoot
    
    & msbuild proj\yamy_test.vcxproj /p:Configuration=$using:Configuration /p:Platform=Win32 /t:Build /v:m /nologo
    if ($LASTEXITCODE -ne 0) { throw "Test Build failed" }
    
    & Release\yamy_test.exe
    if ($LASTEXITCODE -ne 0) { throw "Tests failed" }
}

# 5. Finalize
Invoke-Task "Packaging artifacts" {
    $ErrorActionPreference = "Stop"
    Set-Location $using:RepoRoot
    
    # Renames
    if (Test-Path Release\yamy64) { Move-Item Release\yamy64 Release\yamy64.exe -Force }
    if (Test-Path Release\yamyd32) { Move-Item Release\yamyd32 Release\yamyd32.exe -Force }
    if (Test-Path Release\yamy32) { Move-Item Release\yamy32 Release\yamy32.exe -Force }
    
    # Copy assets
    Copy-Item "keymaps" "Release\keymaps" -Recurse -Force
    Copy-Item "scripts" "Release\scripts" -Recurse -Force
    if (Test-Path "Release\scripts\build_yamy.bat") { Remove-Item "Release\scripts\build_yamy.bat" }
    
    Copy-Item "scripts\launch_yamy.bat" "Release\launch_yamy.bat" -Force
    Copy-Item "scripts\launch_yamy_admin.bat" "Release\launch_yamy_admin.bat" -Force
    if (Test-Path "docs\readme.txt") { Copy-Item "docs\readme.txt" "Release\readme.txt" -Force }
    
    # Cleanup PDB
    Remove-Item "Release\*.pdb" -ErrorAction SilentlyContinue
}

# 6. Zip
Invoke-Task "Creating Distribution Zip" {
    $ErrorActionPreference = "Stop"
    Set-Location $using:RepoRoot
    
    if (-not (Test-Path dist)) { New-Item -ItemType Directory -Path dist -Force | Out-Null }
    if (Test-Path dist\yamy-dist.zip) { Remove-Item dist\yamy-dist.zip -Force }
    
    Compress-Archive -Path "Release\*" -DestinationPath "dist\yamy-dist.zip" -Force
}

Write-Log "`nBuild Complete! Artifact: dist\yamy-dist.zip" "Green"
