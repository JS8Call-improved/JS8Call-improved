#!/bin/bash
set -e

# --- Build a .deb package for JS8Call ---
# --- Run on Ubuntu 24 Server ---
# --- Target: Debian-based systems with glibc 2.39 or later ---

# --- Must not run as root ---
if [ "$(id -u)" -eq 0 ]; then
    echo "This script must NOT be run as root. Exiting."
    exit 1
fi

# --- Variables ---
JS8_ARCH=$(uname -m)
JS8_SOURCE="https://github.com/JS8Call-improved/JS8Call-improved.git"
JS8_BRANCH="master"

# Map uname -m architecture to Debian naming convention
# Debian uses 'amd64' and 'arm64', not 'x86_64' and 'aarch64'
if [ "$JS8_ARCH" = "x86_64" ]; then
    DEB_ARCH="amd64"
elif [ "$JS8_ARCH" = "aarch64" ]; then
    DEB_ARCH="arm64"
else
    echo "Unsupported architecture: $JS8_ARCH"
    exit 1
fi

# --- Tarball URLs ---
JS8LIB_URL="https://github.com/JS8Call-improved/js8lib/releases/download/lib%2F4.0/js8lib4.0-Linux_${JS8_ARCH}_pkg.tar.gz"
QT_URL="https://github.com/JS8Call-improved/js8lib/releases/download/lib%2F4.0/Qt6.11.1_Linux_${JS8_ARCH}_pkg.tar.gz"

# --- Directories ---
BUILD_DIR="$HOME/js8call-deb-build"
STAGING_DIR="$BUILD_DIR/staging"
# This mirrors the target filesystem layout inside the .deb
# When dpkg installs the package, it copies these files to /
INSTALL_PREFIX="/usr/lib/js8call"

# --- Install build dependencies ---
# These are needed to compile JS8Call but will NOT be in the .deb
# The runtime libraries listed in the control file Depends: field
# are what the end user needs — not the -dev packages
echo "Installing build dependencies..."
sudo apt-get update
sudo apt-get install -y \
    build-essential cmake ninja-build perl python3 git wget \
    libssl-dev libfontconfig1-dev libfreetype-dev \
    libharfbuzz-dev libjpeg-dev libpng-dev \
    zlib1g-dev libbrotli-dev libdbus-1-dev libglib2.0-dev \
    libatspi2.0-dev libgl-dev libegl-dev libgbm-dev \
    libdrm-dev libinput-dev libvulkan-dev \
    libxkbcommon-dev libxkbcommon-x11-dev \
    libxcb-util-dev libxcb-image0-dev libxcb-keysyms1-dev \
    libxcb-render-util0-dev libxcb-icccm4-dev libxcb-cursor-dev \
    libxrender-dev libxi-dev \
    libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
    libwayland-dev wayland-protocols \
    libasound2-dev || sudo apt-get install -y libasound2t64

# --- Create build directory structure ---
echo "Creating build directory structure..."
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
mkdir -p "$STAGING_DIR"

# --- Download and extract library tarballs ---
# These go into the build directory temporarily so we can
# build JS8Call against them, and also populate the .deb payload
echo "Downloading js8lib..."
cd "$BUILD_DIR"
wget -q --show-progress "$JS8LIB_URL" -O js8lib.tar.gz
wget -q --show-progress "$QT_URL" -O Qt.tar.gz

# Extract to /usr/lib/js8call so cmake can find them
# This mirrors where they will live on the end user's system
echo "Extracting libraries to $INSTALL_PREFIX..."
sudo mkdir -p "$INSTALL_PREFIX"
sudo tar -xzvf js8lib.tar.gz -C /usr/lib/js8call --strip-components=1
sudo tar -xzvf Qt.tar.gz -C /usr/lib/js8call

# Fix pkgconfig prefix paths to match this machine
# The tarballs were built with /usr/lib/js8call as prefix so
# this should be a no-op, but we run it to be safe
for pc in /usr/lib/js8call/lib/pkgconfig/*.pc; do
    sudo sed -i "s|prefix=.*js8call|prefix=/usr/lib/js8call|g" "$pc"
done
for pc in /usr/lib/js8call/Qt/lib/pkgconfig/*.pc; do
    sudo sed -i "s|prefix=.*js8call/Qt|prefix=/usr/lib/js8call/Qt|g" "$pc"
done

# --- Fetch JS8Call source ---
echo "Fetching JS8Call source..."
cd "$BUILD_DIR"
git clone "$JS8_SOURCE" JS8Call-improved
cd JS8Call-improved
git checkout "$JS8_BRANCH"

# --- Determine version string ---
# Mirrors the logic in CMakeLists.txt:
#   - If VERSION is set to a real semver, use that (release build)
#   - If VERSION is 0.0.0, use the git hash instead (dev build)
# We read VERSION directly from CMakeLists.txt so there's a single
# source of truth — no need to update the script when cutting a release

cd "$BUILD_DIR/JS8Call-improved"

# Extract the version from CMakeLists.txt
CMAKE_VERSION=$(grep -oP 'VERSION\s+\K\d+\.\d+\.\d+' \
    CMakeLists.txt | head -1)

GIT_HASH=$(git rev-parse --short HEAD)

if [ "$CMAKE_VERSION" = "0.0.0" ]; then
    JS8_VERSION="$GIT_HASH"
else
    JS8_VERSION="$CMAKE_VERSION"
fi

echo "Build version: $JS8_VERSION"
PKG_ROOT="$STAGING_DIR/js8call_${JS8_VERSION}_${DEB_ARCH}"
echo "######################################################################"
echo " Building JS8Call $JS8_VERSION .deb for Debian 13"
echo " Architecture: $JS8_ARCH / Debian: $DEB_ARCH"
echo "######################################################################"
sleep 2

# --- Build JS8Call ---
echo "Building JS8Call..."
mkdir build && cd build

cmake \
    -DCMAKE_PREFIX_PATH="/usr/lib/js8call;/usr/lib/js8call/Qt" \
    -DHAMLIB_ROOT="/usr/lib/js8call" \
    -DCMAKE_BUILD_TYPE=Release \
    ..

cmake --build . --parallel $(nproc)

echo "######################################################################"
echo " JS8Call build successful"
echo "######################################################################"
sleep 2

# --- Populate the .deb staging tree ---
# This directory structure mirrors exactly where files will be
# installed on the end user's Debian system when they run:
# sudo dpkg -i js8call_3.0.2_arm64.deb

echo "Populating package staging tree..."

# The binary
mkdir -p "$PKG_ROOT/usr/lib/js8call/bin"
cp JS8Call "$PKG_ROOT/usr/lib/js8call/bin/"

mkdir -p "$PKG_ROOT/usr/bin"
cat > "$PKG_ROOT/usr/bin/JS8Call" << 'EOF'
#!/bin/bash
export LD_LIBRARY_PATH=/usr/lib/js8call/lib:/usr/lib/js8call/Qt/lib
unset KDE_FULL_SESSION
unset KDE_SESSION_VERSION
exec /usr/lib/js8call/bin/JS8Call "$@"
EOF
chmod 755 "$PKG_ROOT/usr/bin/JS8Call"

# The bundled libraries — these are your private Qt and js8lib
# They live in /usr/lib/js8call, not in /usr/lib, so they
# cannot conflict with anything the system package manager owns
mkdir -p "$PKG_ROOT/usr/lib/js8call"
sudo rsync -a /usr/lib/js8call/ "$PKG_ROOT/usr/lib/js8call/"

# Desktop entry so JS8Call appears in the application menu
mkdir -p "$PKG_ROOT/usr/share/applications"
cat > "$PKG_ROOT/usr/share/applications/JS8Call.desktop" << EOF
[Desktop Entry]
Type=Application
Name=JS8Call
Exec=/usr/bin/JS8Call
Icon=js8call
Terminal=false
Categories=HamRadio;Network;
EOF

# Icon
mkdir -p "$PKG_ROOT/usr/share/icons/hicolor/scalable/apps"
cp "$BUILD_DIR/JS8Call-improved/artwork/icon_128.svg" \
    "$PKG_ROOT/usr/share/icons/hicolor/scalable/apps/js8call.svg"

# --- Create the DEBIAN control directory ---
# This is the metadata that dpkg reads to understand the package
mkdir -p "$PKG_ROOT/DEBIAN"

# The control file is the heart of the .deb
# It tells dpkg everything about the package
cat > "$PKG_ROOT/DEBIAN/control" << EOF
Package: js8call
Version: ${JS8_VERSION}
Architecture: ${DEB_ARCH}
Maintainer: JS8Call-improved Project <https://github.com/JS8Call-improved>
Section: hamradio
Homepage: https://github.com/JS8Call-improved/JS8Call-improved
Description: JS8Call digital weak signal communication
 JS8Call is a messaging application built on top of the JS8 digital
 mode, providing keyboard-to-keyboard messaging, store-and-forward
 messaging, and automatic position reporting for amateur radio operators.
Depends: libc6 (>= 2.39), libstdc++6, libgcc-s1,
 libusb-1.0-0, libgl1, libglx0, libglvnd0,
 libxkbcommon0, libpulse0,
 libbrotli1, libzstd1, zlib1g, libudev1,
 libegl1, libfontconfig1, libx11-6, libglib2.0-0,
 libpng16-16, libharfbuzz0b, libfreetype6,
 libpcre2-16-0, libdbus-1-3, libcap2, libexpat1,
 libxcb1, libatomic1, libgraphite2-3, libbz2-1.0,
 libsndfile1, libx11-xcb1, libsystemd0,
 libxau6, libxdmcp6, libopus0, libogg0,
 libmp3lame0, pipewire | pulseaudio
EOF

# postinst runs on the end user's machine AFTER the package files
# are copied into place by dpkg. Updates the desktop menu database.
cat > "$PKG_ROOT/DEBIAN/postinst" << 'EOF'
#!/bin/bash
set -e
if command -v update-desktop-database > /dev/null 2>&1; then
    update-desktop-database /usr/share/applications
fi
EOF
chmod 755 "$PKG_ROOT/DEBIAN/postinst"

# postrm runs AFTER the package is removed
cat > "$PKG_ROOT/DEBIAN/postrm" << 'EOF'
#!/bin/bash
set -e

if command -v update-desktop-database > /dev/null 2>&1; then
    update-desktop-database /usr/share/applications
fi
EOF
chmod 755 "$PKG_ROOT/DEBIAN/postrm"

# --- Build the .deb ---
# dpkg-deb reads the DEBIAN/ directory for metadata and
# packages everything else in the tree as the payload
echo "Building .deb package..."
cd "$STAGING_DIR"
dpkg-deb --build "$PKG_ROOT" "$HOME/js8call_${JS8_VERSION}_${DEB_ARCH}.deb"

echo "######################################################################"
echo " DONE!"
echo " Package: $HOME/js8call_${JS8_VERSION}_${DEB_ARCH}.deb"
echo ""
echo " To install on Debian 13:"
echo "   sudo dpkg -i js8call_${JS8_VERSION}_${DEB_ARCH}.deb"
echo ""
echo " If dependencies are missing:"
echo "   sudo apt-get install -f"
echo "######################################################################"
