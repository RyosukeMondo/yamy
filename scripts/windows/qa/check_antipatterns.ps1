$ErrorActionPreference = "Stop"

$RepoRoot = Resolve-Path "$PSScriptRoot/../../.."
$SrcRoot = "$RepoRoot/src"

Write-Host "Checking for code anti-patterns in: $SrcRoot" -ForegroundColor Cyan

$AntiPatterns = @(
    @{
        Name         = "Legacy NULL";
        Pattern      = "\bNULL\b";
        Suggestion   = "Use 'nullptr' instead of 'NULL'.";
        Type         = "Error";
        ExcludePaths = @("src/tests/", "src/ts4mayu/")
    },
    @{
        Name          = "Namespace Pollution";
        Pattern       = "^using\s+namespace\s+std;";
        Suggestion    = "Do not use 'using namespace std;' in header files.";
        Type          = "Error";
        FileExtension = ".h";
        ExcludePaths  = @("src/tests/")
    },
    @{
        Name         = "Unsafe String Function";
        Pattern      = "\b(sprintf|strcpy|strcat)\b";
        Suggestion   = "Use safe alternatives like 'snprintf', 'strncpy', or std::string.";
        Type         = "Warning";
        ExcludePaths = @("src/tests/", "src/ts4mayu/")
    },
    @{
        Name         = "Raw Memory Allocation";
        Pattern      = "\b(malloc|free)\b";
        Suggestion   = "Use 'new'/'delete' or smart pointers.";
        Type         = "Warning";
        ExcludePaths = @("src/tests/", "src/ts4mayu/", "src/platform/")
    },
    @{
        Name         = "Source File Inclusion";
        Pattern      = '^\s*#include\s+["<].+\.cpp[">]';
        Suggestion   = "Do not include .cpp files.";
        Type         = "Error";
        ExcludePaths = @("src/tests/")
    },
    @{
        Name         = "Conditional Compilation in Core";
        Pattern      = '^\s*#ifn?def\s+';
        Suggestion   = "Minimize #ifdef in core logic. Use interfaces/polymorphism.";
        Type         = "Warning";
        ExcludePaths = @("src/tests/", "src/ts4mayu/", "src/platform/", "src/core/engine/engine.h") # Engine is legacy central
    },
    @{
        Name         = "Tab Character";
        Pattern      = "\t";
        Suggestion   = "Use spaces instead of tabs.";
        Type         = "Warning"; 
        ExcludePaths = @("src/tests/", "src/ts4mayu/") 
    }
)

$Files = Get-ChildItem -Path $SrcRoot -Recurse -Include *.cpp, *.h, *.hpp, *.c | Where-Object { $_.FullName -notmatch "googletest" }

$ErrorCount = 0
$WarningCount = 0

foreach ($file in $Files) {
    $relativePath = $file.FullName.Substring($RepoRoot.Path.Length + 1).Replace("\", "/")
    
    # Read file content efficiently
    $content = Get-Content $file.FullName -Raw
    if (-not $content) { continue }

    foreach ($check in $AntiPatterns) {
        # Check exclusions
        $skip = $false
        if ($check.ExcludePaths) {
            foreach ($exclude in $check.ExcludePaths) {
                if ($relativePath -cmatch $exclude) { $skip = $true; break }
            }
        }
        if ($skip) { continue }

        # Check extensions
        if ($check.FileExtension -and $file.Extension -ne $check.FileExtension) { continue }

        if ($content -cmatch $check.Pattern) {
            # Find line number (slower, so only do it on match)
            $lines = $content -split "`r`n"
            for ($i = 0; $i -lt $lines.Count; $i++) {
                if ($lines[$i] -cmatch $check.Pattern) {
                    $msg = "[$($check.Type)] $($check.Name): $relativePath`:($($i+1)) - $($check.Suggestion)"
                    if ($check.Type -eq "Error") {
                        Write-Host $msg -ForegroundColor Red
                        $ErrorCount++
                    }
                    else {
                        Write-Host $msg -ForegroundColor Yellow
                        $WarningCount++
                    }
                    # breaking after first match per file/check to avoid spam? No, show all context.
                }
            }
        }
    }
}

Write-Host "Anti-pattern check complete."
Write-Host "Errors: $ErrorCount" -ForegroundColor $(if ($ErrorCount -gt 0) { "Red" } else { "Green" })
Write-Host "Warnings: $WarningCount" -ForegroundColor $(if ($WarningCount -gt 0) { "Yellow" } else { "Green" })

if ($ErrorCount -gt 0) {
    Write-Host "Found $ErrorCount anti-pattern errors. Build failed." -ForegroundColor Red
    exit 1
}

exit 0
