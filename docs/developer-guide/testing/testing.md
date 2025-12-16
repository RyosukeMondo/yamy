# YAMY One-Click Testing

## Quick Start (Just 3 Steps!)

### Step 1: Download Test Script
```powershell
# Create a test folder
mkdir C:\yamy-test
cd C:\yamy-test

# Download the scripts
curl -O https://raw.githubusercontent.com/RyosukeMondo/yamy/master/scripts/windows/test-yamy.bat
curl -O https://raw.githubusercontent.com/RyosukeMondo/yamy/master/scripts/windows/test-yamy.ps1
```

### Step 2: Run the Script
```
Double-click: test-yamy.bat
```

### Step 3: Test and Share Logs
- Try clicking the tray icon
- Share the console output or `%LOCALAPPDATA%\YAMY\yamy.log`

---

## What the Script Does

1. ✅ Downloads YAMY v1.0.3 from GitHub
2. ✅ Extracts to `%TEMP%\yamy-test`
3. ✅ Launches in debug mode (shows logs)
4. ✅ Fresh test environment every time

## Alternative: Direct Download

If you prefer manual testing:
- Download: https://github.com/RyosukeMondo/yamy/releases/download/v1.0.3/yamy-dist.zip
- Extract and run `launch_yamy_debug.bat`

## Version Strategy

We're using **single version v1.0.3** for all testing:
- ✅ No version confusion
- ✅ Just re-run test script for latest changes
- ✅ Same download link always works

---

**Next Iteration:**
Just run `test-yamy.bat` again - it automatically gets the latest v1.0.3!
