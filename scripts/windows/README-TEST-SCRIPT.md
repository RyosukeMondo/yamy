# YAMY One-Click Test Script

Automatically downloads, extracts, and launches YAMY v1.0.3 for quick testing.

## Quick Start

### Option 1: Double-Click (Easiest)
```
Just double-click: test-yamy.bat
```

### Option 2: PowerShell (More Control)
```powershell
# Debug mode (default - shows console logs)
.\test-yamy.ps1

# Admin mode
.\test-yamy.ps1 -Mode admin

# Normal mode
.\test-yamy.ps1 -Mode normal
```

## What It Does

1. **Cleans up** - Removes `%TEMP%\yamy-test` if it exists
2. **Downloads** - Gets YAMY v1.0.3 from GitHub releases
3. **Extracts** - Unzips to `%TEMP%\yamy-test`
4. **Launches** - Runs the appropriate .bat file

## Test Modes

| Mode | Command | Description |
|------|---------|-------------|
| **Debug** | `test-yamy.ps1 -Mode debug` | Shows console with real-time logs (recommended) |
| **Admin** | `test-yamy.ps1 -Mode admin` | Runs as administrator (UAC prompt) |
| **Normal** | `test-yamy.ps1 -Mode normal` | Standard user mode |

## After Testing

- **Logs**: Check `%LOCALAPPDATA%\YAMY\yamy.log`
- **Test folder**: Located at `%TEMP%\yamy-test`
- **Clean up**: Just delete `%TEMP%\yamy-test` when done

## Benefits

✅ No manual download/extract needed
✅ Always gets the latest v1.0.3
✅ Fresh test environment every time
✅ Doesn't interfere with existing YAMY installations
✅ One command to test iterations

## Troubleshooting

**"Execution policy error"**
```powershell
Set-ExecutionPolicy -Scope CurrentUser RemoteSigned
```

**"Download failed"**
- Check internet connection
- Verify GitHub is accessible
- Check if v1.0.3 release exists

**"Access denied"**
- Close any running YAMY instances
- Try running as administrator
