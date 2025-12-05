$ErrorActionPreference = "Stop"
$url = "https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip"
$zipPath = "$PSScriptRoot\googletest.zip"
$extractPath = "$PSScriptRoot\googletest_temp"
$destPath = "$PSScriptRoot\..\src\tests\googletest"

Write-Host "Downloading Google Test from $url..."
Invoke-WebRequest -Uri $url -OutFile $zipPath

Write-Host "Extracting..."
Expand-Archive -Path $zipPath -DestinationPath $extractPath -Force

Write-Host "Installing to $destPath..."
if (Test-Path $destPath) {
    Remove-Item -Recurse -Force $destPath
}

# The zip contains googletest-1.14.0/googletest
$source = Join-Path $extractPath "googletest-1.14.0" | Join-Path -ChildPath "googletest"
Move-Item -Path $source -Destination $destPath

Write-Host "Cleanup..."
Remove-Item $zipPath
Remove-Item -Recurse -Force $extractPath

Write-Host "Google Test setup complete."
