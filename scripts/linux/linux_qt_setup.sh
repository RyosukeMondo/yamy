#!/bin/bash
# YAMY Linux Qt GUI Setup Script
# Automates installation of Qt5 and dependencies

set -e

echo "========================================="
echo "YAMY Linux Qt GUI Setup"
echo "========================================="
echo ""

# Detect Linux distribution
if [ -f /etc/os-release ]; then
    . /etc/os-release
    DISTRO=$ID
else
    echo "Error: Cannot detect Linux distribution"
    exit 1
fi

echo "Detected distribution: $DISTRO"
echo ""

# Install packages based on distribution
case "$DISTRO" in
    ubuntu|debian|pop|linuxmint)
        echo "Installing packages for Ubuntu/Debian..."
        sudo apt update
        sudo apt install -y \
            build-essential \
            cmake \
            qtbase5-dev \
            qttools5-dev \
            libqt5x11extras5-dev \
            libx11-dev \
            libxrandr-dev \
            libudev-dev
        ;;

    fedora|rhel|centos)
        echo "Installing packages for Fedora/RHEL..."
        sudo dnf install -y \
            gcc-c++ \
            cmake \
            qt5-qtbase-devel \
            qt5-qttools-devel \
            qt5-qtx11extras-devel \
            libX11-devel \
            libXrandr-devel \
            libudev-devel
        ;;

    arch|manjaro)
        echo "Installing packages for Arch Linux..."
        sudo pacman -S --needed --noconfirm \
            base-devel \
            cmake \
            qt5-base \
            qt5-tools \
            qt5-x11extras \
            libx11 \
            libxrandr \
            systemd-libs
        ;;

    *)
        echo "Error: Unsupported distribution: $DISTRO"
        echo "Please install Qt5 manually and run cmake -B build"
        exit 1
        ;;
esac

echo ""
echo "========================================="
echo "Verifying Qt5 installation..."
echo "========================================="

# Verify Qt5
if command -v qmake &> /dev/null; then
    qmake --version
    echo "✓ Qt5 installed successfully"
else
    echo "✗ Qt5 installation failed"
    exit 1
fi

echo ""
echo "========================================="
echo "Building YAMY with Qt GUI..."
echo "========================================="

# Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

echo ""
echo "========================================="
echo "Setup Complete!"
echo "========================================="
echo ""
echo "To run YAMY Qt GUI:"
echo "  ./build/bin/yamy_stub"
echo ""
echo "For more information, see:"
echo "  docs/LINUX-QT-GUI-MANUAL.md"
echo ""
