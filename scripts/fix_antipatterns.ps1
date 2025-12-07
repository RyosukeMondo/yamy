$ErrorActionPreference = "Stop"

$RepoRoot = Resolve-Path "$PSScriptRoot/.."
$SrcRoot = "$RepoRoot/src"

Write-Host "Fixing code anti-patterns in: $SrcRoot" -ForegroundColor Cyan

# Configuration
$TabReplacement = "    " # 4 spaces
$NullPattern = "\bNULL\b"
$NullReplacement = "nullptr"

# Exclusion list (Regex pattern to match relative path)
# engine is legacy but we should probably try to clean it too, sticking to plan's exclusions
$ExcludePatterns = @(
    "src/tests/",
    "src/ts4mayu/" 
)

$Files = Get-ChildItem -Path $SrcRoot -Recurse -Include *.cpp, *.h, *.hpp, *.c | Where-Object { $_.FullName -notmatch "googletest" }

$FixedCount = 0
$TabCount = 0
$NullCount = 0

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

    $originalContent = $content
    $modified = $false

    # 1. Fix Tabs
    if ($content -match "\t") {
        $content = $content -replace "\t", $TabReplacement
        $TabCount++
        $modified = $true
    }

    # 2. Fix NULL
    if ($content -match $NullPattern) {
        $content = $content -replace $NullPattern, $NullReplacement
        $NullCount++
        $modified = $true
    }

    if ($modified) {
        Set-Content -Path $file.FullName -Value $content -NoNewline
        Write-Host "Fixed: $relativePath" -ForegroundColor Green
        $FixedCount++
    }
}

Write-Host "Fix complete."
Write-Host "Files updated: $FixedCount" -ForegroundColor Cyan
Write-Host "  - Files with Tabs fixed (rough count): $TabCount"
Write-Host "  - Files with NULL fixed (rough count): $NullCount"
