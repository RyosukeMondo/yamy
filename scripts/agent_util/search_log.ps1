param(
    [string]$LogFile = "build_log_x64.txt",
    [string]$Pattern = "error",
    [int]$Context = 2,
    [int]$Limit = 20,
    [switch]$All
)

if (-not (Test-Path $LogFile)) {
    Write-Error "Log file not found: $LogFile"
    exit 1
}

# Use Select-String which handles encoding detection fairly well in newer PowerShell,
# but for older versions or specific mix-ups, we rely on default behavior or add -Encoding if needed.
# build_log_x64.txt from redirection is usually UTF-16LE or UTF-8.

$results = Select-String -Path $LogFile -Pattern $Pattern -Context $Context

if ($results) {
    if (-not $All -and $Limit -gt 0) {
        $results | Select-Object -First $Limit
    } else {
        $results
    }
} else {
    Write-Host "No matches found for '$Pattern' in $LogFile"
}
