# Project Overview
- Purpose: Yamy is a key binding customization tool (Qt5 GUI) for Windows and Linux, forked from Mayu. Uses user-mode keyboard hook + SendInput; includes yamy GUI/tray app and yamy-ctl CLI.
- Structure: src/ (core engine, platform abstractions, UI dialogs/resources, system hooks, utils, app entrypoints), proj/ external deps, driver/ device driver code, keymaps/ sample .mayu configs, resources/ icons/cursors, scripts/ build/test utilities, setup/ installers, docs/ docs.
- Build targets: Qt5 GUI binaries `yamy` and CLI `yamy-ctl`; Linux stub support. Build via CMake or packaging scripts.
- Notable docs/scripts: README.md (build/install), quick-test.sh (IPC sanity run from build directory), TESTING.md and MANUAL-TEST-GUIDE.md (manual testing), RELEASE-NOTES-1.0.md for distro-specific install.
- CI: GitHub Actions for Linux/Windows builds and static analysis (no Windows deps in src/core).