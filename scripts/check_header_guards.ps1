
# scripts/check_header_guards.ps1
# Checks if all header files (*.h, *.hpp) start with #pragma once
# Excluding 3rd party or specific directories

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot = Split-Path -Parent $ScriptDir
$SrcDir = Join-Path $RepoRoot "src"

Write-Host "Checking for #pragma once in headers..." -ForegroundColor Cyan

# Find all header files
$files = Get-ChildItem -Path $SrcDir -Recurse -Include "*.h","*.hpp" | Select-Object -ExpandProperty FullName

$missingGuard = @()

foreach ($file in $files) {
    # Relative path for cleaner output/matching
    $relativePath = $file.Substring($RepoRoot.Length + 1).Replace("\", "/")
    
    # Exclude tests, legacy, or specific friend-injection headers
    if ($relativePath -match "^src/tests/" -or 
        $relativePath -match "^src/ts4mayu/" -or 
        $relativePath -match "function_friends.h") {
         continue
    }

    # Read the first few lines to check for pragma once
    # We read raw to avoid encoding issues for this simple check, 
    # but Get-Content -TotalCount is efficient.
    $content = Get-Content -Path $file -TotalCount 20

    $hasPragma = $false
    foreach ($line in $content) {
        if ($line.Trim() -eq "#pragma once") {
            $hasPragma = $true
            break
        }
        # If we hit code/ifdef before pragma once, it's likely missing or misplaced (ignoring comments)
        if ($line.Trim().StartsWith("#ifndef") -or $line.Trim().StartsWith("class")) {
             # Heuristic: old Include Guard style or missing guard
        }
    }

    if (-not $hasPragma) {
        $missingGuard += $relativePath
    }
}

if ($missingGuard.Count -gt 0) {
    Write-Host "Found $($missingGuard.Count) header files missing '#pragma once':" -ForegroundColor Red
    foreach ($missing in $missingGuard) {
        Write-Host "  - $missing" -ForegroundColor Red
    }
    Write-Host "`nPlease add '#pragma once' to the top of these files." -ForegroundColor Yellow
    exit 1
} else {
    Write-Host "All headers have #pragma once." -ForegroundColor Green
    exit 0
}
