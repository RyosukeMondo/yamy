# Suggested Commands
- Configure Release build with GUI + Linux stub: `cmake -B build_release -DCMAKE_BUILD_TYPE=Release -DBUILD_LINUX_STUB=ON -DBUILD_QT_GUI=ON -DBUILD_LINUX_TESTING=OFF -DBUILD_REGRESSION_TESTS=OFF -DCMAKE_INSTALL_PREFIX=/usr` (run at repo root).
- Build after configure: `cmake --build build_release -j$(nproc)`; binaries land in `build_release/bin/`.
- Package on Linux: `./scripts/linux/build_linux_package.sh [VERSION]` (creates tarball in dist/).
- Run quick IPC sanity test (from build dir with binaries present): `./quick-test.sh` (requires `bin/yamy` and `bin/yamy-ctl`).
- Manual launch (after build): `build_release/bin/yamy --no-restore` and control via `build_release/bin/yamy-ctl {status|config|keymaps|metrics|reload <config>|start|stop}`.
- Legacy migration metrics: `bash scripts/linux/track_legacy_strings.sh`.
- Windows packaging: `scripts/windows/cmake_package.ps1 [-Clean]`; MinGW packaging: `scripts/mingw_package.ps1`.
- Manual build on Windows via CMake preset scripts is available; see README for steps.