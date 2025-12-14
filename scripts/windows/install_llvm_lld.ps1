# Install LLVM (clang-cl + lld-link) on Windows
param(
    [ValidateSet("winget", "choco")]
    [string]$PackageManager = "winget",
    [string]$Version = "",
    [switch]$Force
)

function Install-WithWinget {
    $args = @("install", "--id", "LLVM.LLVM", "--source", "winget", "--accept-package-agreements", "--accept-source-agreements")
    if ($Version) { $args += @("--version", $Version) }
    if ($Force) { $args += "--force" }
    winget @args
}

function Install-WithChoco {
    $args = @("install", "llvm", "-y")
    if ($Version) { $args += @("--version", $Version) }
    if ($Force) { $args += "--force" }
    choco @args
}

if ($PackageManager -eq "winget") {
    Install-WithWinget
} else {
    Install-WithChoco
}

$llvmRoot = "${env:ProgramFiles}\LLVM"
$lldPath = Join-Path $llvmRoot "bin\lld-link.exe"

if (Test-Path $lldPath) {
    Write-Host "lld-link installed at $lldPath"
    Write-Host "Add $llvmRoot\bin to PATH to let CMake find clang-cl and lld-link."
} else {
    Write-Warning "LLVM install completed, but lld-link.exe was not found under $llvmRoot. Check installer output."
}
