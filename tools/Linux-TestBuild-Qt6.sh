#!/bin/bash
set -e

####### Build Qt6 for JS8Call #######
####### Ubuntu 24.04 Server only  #######

# --- Qt version to build ---
QT_VERSION="6.11.1"
QT_TAG="v${QT_VERSION}"

clear
echo -e "This script is only for building Qt from source on linux
for testing purposes. Do NOT use it to build a distribution Qt package.
It will build and install Qt to ~/.local/lib/Qt and create a tarball
of your Qt build that can be saved and unpacked for testing JS8Call
builds. In the header of this script you can define the version of Qt
you wish to download and build.

This script is restricted to building on Ubuntu 24 Server to ensure your
Qt build is glibc-portable. If you wish to build Qt on a different platform
you must comment out the 'Restrict to Ubuntu 24.04 only' section in the script."
read -p "Press Enter to continue" </dev/tty

# --- Detect distro ---
if [ -f /etc/os-release ]; then
    . /etc/os-release
    DISTRO=$ID
else
    echo "Cannot detect Linux distribution. Exiting."
    exit 1
fi

# --- Restrict to Ubuntu 24.04 only ---
if [[ "$DISTRO" != "ubuntu" ]]; then
  echo "This script is only supported on Ubuntu 24.04 Server."
  echo "Detected: $DISTRO"
  exit 1
fi

if [[ "$VERSION_ID" != "24.04" ]]; then
  echo "This script requires Ubuntu 24.04 Server."
  echo "Detected Ubuntu version: $VERSION_ID"
  exit 1
fi

BUILD_ARCH=$(uname -m)
if [[ "$BUILD_ARCH" != "x86_64" && "$BUILD_ARCH" != "aarch64" ]]; then
  echo "Unsupported architecture: $BUILD_ARCH"
  echo "This script supports x86_64 and aarch64 only."
  exit 1
fi

echo "######################################################################"
echo "Build environment verified: Ubuntu $VERSION_ID / $BUILD_ARCH"
echo "######################################################################"

install_deps() {
    echo "Installing build dependencies for Ubuntu $VERSION_ID / $BUILD_ARCH..."
    sudo apt-get update
    sudo apt-get install -y \
        cmake ninja-build g++ perl python3 git \
        libssl-dev libfontconfig1-dev libfreetype-dev \
        libharfbuzz-dev libjpeg-dev libpng-dev \
        zlib1g-dev libbrotli-dev libdbus-1-dev libglib2.0-dev \
        libatspi2.0-dev \
        libwayland-dev wayland-protocols \
        libxkbcommon-dev libxkbcommon-x11-dev \
        libgl-dev libegl-dev libgbm-dev \
        libdrm-dev libinput-dev \
        libxcb1-dev libx11-xcb-dev \
        libxcb-glx0-dev libxcb-xkb-dev \
        libxcb-icccm4-dev libxcb-image0-dev \
        libxcb-keysyms1-dev libxcb-render-util0-dev \
        libxcb-xinerama0-dev libxcb-xfixes0-dev \
        libxcb-shape0-dev libxcb-randr0-dev \
        libxcb-cursor-dev libxcb-sync-dev \
        libxcb-util-dev \
        libxkbcommon-x11-dev \
        libx11-dev \
        libxrender-dev libxi-dev \
        libxrandr-dev libxext-dev libxfixes-dev \
        libxshmfence-dev \
        libpulse-dev libasound2-dev \
        libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
        yasm nasm \
        clang libclang-dev llvm llvm-dev \
        libpcre2-dev
}

install_deps

# --- FFmpeg ---
mkdir -p ~/development
cd ~/development
git clone --branch n7.1.3 https://git.ffmpeg.org/ffmpeg.git
cd ffmpeg
./configure \
  --prefix=/usr/local/ffmpeg \
  --enable-gpl \
  --enable-shared \
  --disable-static
make -j$(nproc)
sudo make install

export PKG_CONFIG_PATH=/usr/local/ffmpeg/lib/pkgconfig:$PKG_CONFIG_PATH

# --- Qt6 source ---
cd ~/development
git clone https://github.com/qt/qt5.git Qt6
cd Qt6
git checkout ${QT_TAG}
./init-repository --module-subset=qtbase,qtshadertools,qtmultimedia,\
qtimageformats,qtserialport,qtsvg,qtwebsockets,qtwayland,\
qtdeclarative,qttools,qtpositioning,qttranslations,qtlanguageserver,\
qtquicktimeline,qt5compat

# --- Configure ---
mkdir -p ~/development/qt6-build
cd ~/development/qt6-build

export CC=clang
export CXX=clang++
export PKG_CONFIG_PATH=/usr/local/ffmpeg/lib/pkgconfig:$PKG_CONFIG_PATH

../Qt6/configure \
  -prefix ~/.local/lib/Qt \
  -release \
  -shared \
  -opensource \
  -confirm-license \
  -platform linux-clang \
  -DFEATURE_clang=ON \
  -ffmpeg-dir /usr/local/ffmpeg \
  -ffmpeg-deploy \
  -skip qtquick3d \
  -no-feature-testlib \
  -bundled-xcb-xinput

# --- Build and install ---
cmake --build . --parallel $(nproc)
cmake --install .

echo "Qt${QT_VERSION} build complete. Installed to ~/.local/lib/Qt"
echo "######################################################################"

# --- Bundle libicu ---
echo "Bundling libicu libraries for $BUILD_ARCH..."

if [ "$BUILD_ARCH" = "aarch64" ]; then
  ICU_LIB_PATH="/usr/lib/aarch64-linux-gnu"
else
  ICU_LIB_PATH="/usr/lib/x86_64-linux-gnu"
fi

cp -P ${ICU_LIB_PATH}/libicui18n.so* ~/.local/lib/Qt/lib/
cp -P ${ICU_LIB_PATH}/libicuuc.so* ~/.local/lib/Qt/lib/
cp -P ${ICU_LIB_PATH}/libicudata.so* ~/.local/lib/Qt/lib/

echo "libicu libraries bundled successfully."
echo "######################################################################"

# --- Create tar.gz archive ---
TARBALL="Qt${QT_VERSION}_Linux_${BUILD_ARCH}.tar.gz"
echo "Creating archive $HOME/${TARBALL}..."
tar -czf "$HOME/${TARBALL}" -C "$HOME/.local/lib" Qt

echo "######################################################################"
