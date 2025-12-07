$logFile = "build_log_x64.txt"
if (Test-Path $logFile) {
    echo "Scanning $logFile for errors..."
    Select-String -Path $logFile -Pattern ": error" -Context 3,3 | ForEach-Object {
        echo "--------------------------------------------------"
        echo $_.Context.PreContext
        echo $_.Line
        echo $_.Context.PostContext
    }
} else {
    echo "Log file $logFile not found."
}
