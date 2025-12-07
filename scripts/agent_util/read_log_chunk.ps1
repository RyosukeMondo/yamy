param(
    [string]$LogFile = "build_log_x64.txt",
    [int]$StartLine = 1,
    [int]$Count = 50
)

if (-not (Test-Path $LogFile)) {
    Write-Error "Log file not found: $LogFile"
    exit 1
}

# Get-Content is 1-indexed for TotalCount/Head but we need skip for start line
# 0-indexed skip. If StartLine is 1, skip 0.
$skip = $StartLine - 1

if ($skip -lt 0) { $skip = 0 }

Get-Content -Path $LogFile | Select-Object -Skip $skip -First $Count
