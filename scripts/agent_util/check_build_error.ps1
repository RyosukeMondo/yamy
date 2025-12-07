param(
    [string]$LogFile = "build_log_x64.txt",
    [int]$Context = 2,
    [int]$Limit = 15
)

if (-not (Test-Path $LogFile)) {
    Write-Error "Log file not found: $LogFile"
    exit 1
}

# Find lines containing "error" or "Error" (case-insensitive by default in Select-String)
# showing context lines.
$results = Select-String -Path $LogFile -Pattern "error" -Context $Context

if ($results) {
    # If limit is set, take only the first N
    if ($Limit -gt 0) {
        $results | Select-Object -First $Limit
    } else {
        $results
    }
} else {
    Write-Host "No errors found in $LogFile"
}
