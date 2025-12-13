$sessionsFile = "sessions_wave2_update.txt"
$promptsDir = "."

if (-not (Test-Path $sessionsFile)) {
    # Try generating it if missing
    jules remote list --session > $sessionsFile
}

$sessions = Get-Content $sessionsFile
$prompts = Get-ChildItem "prompt_wave2_*.txt"

$results = @()

foreach ($line in $sessions) {
    if ($line -match "(\d+)\s+(.+?)\s+(RyosukeMondo/yamy)") {
        $id = $matches[1]
        $desc = $matches[2]
        $file = "Unknown"

        # Try to match description to prompt content
        foreach ($p in $prompts) {
            $content = Get-Content $p.FullName -Raw
            # Extract Task name from prompt
            if ($content -match "Task: (.+)") {
                $taskName = $matches[1].Trim()
                # Check if description contains the task name
                # Jules description is roughly "Role: ... Task: [TaskName]..."
                if ($desc -like "*$taskName*") {
                    $file = $p.Name
                    break
                }
            }
        }
        
        $results += [PSCustomObject]@{
            ID          = $id
            File        = $file
            Description = $desc.Substring(0, [Math]::Min(30, $desc.Length)) + "..."
        }
    }
}

$results | Format-Table -AutoSize
