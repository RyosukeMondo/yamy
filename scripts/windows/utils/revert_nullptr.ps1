$ErrorActionPreference = "Stop"

$RepoRoot = Resolve-Path "$PSScriptRoot/.."
$SrcRoot = "$RepoRoot/src"

Write-Host "Reverting nullptr to NULL in: $SrcRoot" -ForegroundColor Cyan

# Configuration
$NullptrPattern = "nullptr"
$NullReplacement = "NULL"

# Exclusion list (Same as before to be safe)
$ExcludePatterns = @(
    "src/tests/",
    "src/ts4mayu/" 
)

$Files = Get-ChildItem -Path $SrcRoot -Recurse -Include *.cpp, *.h, *.hpp, *.c | Where-Object { $_.FullName -notmatch "googletest" }

$RevertedCount = 0

foreach ($file in $Files) {
    $relativePath = $file.FullName.Substring($RepoRoot.Path.Length + 1).Replace("\", "/")
    
    # Check exclusions
    $skip = $false
    foreach ($exclude in $ExcludePatterns) {
        if ($relativePath -match $exclude) { $skip = $true; break }
    }
    if ($skip) { continue }

    $content = Get-Content $file.FullName -Raw
    if (-not $content) { continue }

    if ($content -match $NullptrPattern) {
        $content = $content -replace $NullptrPattern, $NullReplacement
        Set-Content -Path $file.FullName -Value $content -NoNewline
        Write-Host "Reverted: $relativePath" -ForegroundColor Green
        $RevertedCount++
    }
}

Write-Host "Revert complete."
Write-Host "Files reverted: $RevertedCount" -ForegroundColor Cyan
