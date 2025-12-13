
# scripts/check_encoding.ps1
# Checks if source files (*.cpp, *.h) are UTF-8 with BOM
# MSVC requires BOM for source files containing non-ASCII chars to compile correctly across locales.

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot = Split-Path -Parent (Split-Path -Parent $ScriptDir)
$SrcDir = Join-Path $RepoRoot "src"

Write-Host "Checking for UTF-8 BOM in source files..." -ForegroundColor Cyan

# Find all relevant source files
$files = Get-ChildItem -Path $SrcDir -Recurse -Include "*.h", "*.hpp", "*.cpp", "*.c" | Select-Object -ExpandProperty FullName

$badEncoding = @()

foreach ($file in $files) {
    # Relative path
    $relativePath = $file.Substring($RepoRoot.Length + 1).Replace("\", "/")
    
    # Exclude 3rd party/tests if needed
    if ($relativePath -match "^src/tests/" -or $relativePath -match "^src/ts4mayu/") {
        continue
    }

    # Helper function to check BOM
    # UTF-8 BOM is 0xEF, 0xBB, 0xBF
    # UTF-8 BOM is 0xEF, 0xBB, 0xBF
    $bytes = $null
    if ($PSVersionTable.PSVersion.Major -ge 6) {
        $bytes = Get-Content -Path $file -AsByteStream -TotalCount 3
    }
    else {
        $bytes = Get-Content -Path $file -Encoding Byte -TotalCount 3
    }
    if ($null -eq $bytes -or $bytes.Count -lt 3) {
        # Empty or small files might be fine, or might be empty.
        # Let's ignore files < 3 bytes for BOM check
        continue
    }

    if ($bytes[0] -ne 0xEF -or $bytes[1] -ne 0xBB -or $bytes[2] -ne 0xBF) {
        # Check if it contains non-ASCII characters? 
        # Strictly speaking, if it's ASCII-only, MSVC handles it fine without BOM.
        # But for consistency in this project, we might enforce BOM or at least warn.
        # For a "cheap" check, let's just list it.
        $badEncoding += $relativePath
    }
}

if ($badEncoding.Count -gt 0) {
    Write-Host "Found $($badEncoding.Count) files missing UTF-8 BOM:" -ForegroundColor Red
    foreach ($missing in $badEncoding) {
        Write-Host "  - $missing" -ForegroundColor Red
    }
    Write-Host "`nMSVC may misinterpret non-ASCII characters in these files." -ForegroundColor Yellow
    Write-Host "Consider converting them to UTF-8 with BOM." -ForegroundColor Yellow
    exit 1
}
else {
    Write-Host "All source files have UTF-8 BOM." -ForegroundColor Green
    exit 0
}
