$ErrorActionPreference = "Stop"

$RepoRoot = Resolve-Path "$PSScriptRoot/../.."
$SrcRoot = "$RepoRoot/src"

Write-Host "Fixing UTF-8 BOM in source files: $SrcRoot" -ForegroundColor Cyan

# Configuration
$Extensions = @(".cpp", ".c", ".h", ".hpp")
$ExcludePatterns = @(
    "src/tests/"
    "src/ts4mayu/"
)

# Force UTF-8 with BOM
$Utf8WithBom = New-Object System.Text.UTF8Encoding $true

$Files = Get-ChildItem -Path $SrcRoot -Recurse | Where-Object { 
    $ext = $_.Extension.ToLower()
    $Extensions -contains $ext
}

$FixedCount = 0

foreach ($file in $Files) {
    $relativePath = $file.FullName.Substring($RepoRoot.Path.Length + 1).Replace("\", "/")
    
    # Check exclusions
    $skip = $false
    foreach ($exclude in $ExcludePatterns) {
        if ($relativePath -match $exclude) { $skip = $true; break }
    }
    if ($skip) { continue }

    # logic to check if needs fix or just force fix?
    # Forcing fix is safer to ensure consistency, but let's check first to avoid unnecessary writes
    
    $bytes = $null
    if ($PSVersionTable.PSVersion.Major -ge 6) {
        $bytes = Get-Content -Path $file.FullName -AsByteStream -TotalCount 3
    }
    else {
        $bytes = Get-Content -Path $file.FullName -Encoding Byte -TotalCount 3
    }
    $hasBom = ($null -ne $bytes -and $bytes.Count -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF)
    
    if (-not $hasBom) {
        $content = [System.IO.File]::ReadAllText($file.FullName)
        [System.IO.File]::WriteAllText($file.FullName, $content, $Utf8WithBom)
        Write-Host "Fixed (Added BOM): $relativePath" -ForegroundColor Green
        $FixedCount++
    }
}

Write-Host "Encoding fix complete."
Write-Host "Files updated: $FixedCount" -ForegroundColor Cyan
