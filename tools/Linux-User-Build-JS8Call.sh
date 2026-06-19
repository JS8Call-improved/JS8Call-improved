#!/bin/bash
set -e

# (C) copyright 2025 Chris Olson AC9KH, Joseph Counsil K0OG
# Builds and installs JS8Call from source on a linux system.
# Requires sudo access. Installs to /usr/lib/js8call and /usr/bin.

# --- Variables ---
red="\033[0;31m"
clear_color="\033[0m"
JS8_VERSION="master"
JS8_INSTALL_PREFIX="/usr/lib/js8call"
JS8_QT_DIR="${JS8_INSTALL_PREFIX}/Qt"

# Legacy install paths — used to detect and remove old ~/.local installs
LEGACY_BIN="$HOME/.local/bin/JS8Call"
LEGACY_QT_DIR="$HOME/.local/lib/Qt"
LEGACY_LIB_DIR="$HOME/.local/lib/js8lib"
LEGACY_DESKTOP="$HOME/.local/share/applications/JS8Call.desktop"
LEGACY_ICON="$HOME/.local/share/icons/icon_128.svg"

# --- Functions ---
divider() {
  echo "######################################################################"
}

error() {
  echo "Installation has exited with an error."
  exit 1
}

clear

# --- Must not run as root ---
if [ "$(id -u)" -eq 0 ]; then
  divider
  echo -e "${red}You are logged in as root.

You MUST be logged in as a normal user to run this script.

It is recommended to add your username to the sudo group. We need to
install system libraries that are required to build JS8Call. This must
be done as a sudo user.

This script will NOT RUN AS ROOT!${clear_color}"
  divider
  exit 1
fi

# --- Detect distro ---
if [ -f /etc/os-release ]; then
  . /etc/os-release
  DISTRO=$ID
else
  echo "Cannot detect Linux distribution. Exiting."
  exit 1
fi

# --- Check for .deb managed installation ---
if dpkg -l js8call 2>/dev/null | grep -q "^ii"; then
  divider
  echo -e "${red}JS8Call is currently installed via the .deb package.

Running this script will overwrite the package-managed installation
and may cause conflicts with your package manager. It is strongly
recommended to remove the .deb package first:

  sudo dpkg -r js8call

This ensures a clean installation without package manager conflicts.${clear_color}"
  divider
  read -p "Remove the .deb package now and continue? Yes(y) / No(n): " REMOVE_DEB </dev/tty
  if [ "${REMOVE_DEB}" = "y" ]; then
    sudo dpkg -r js8call
    divider
    echo "Package removed. Continuing with build installation..."
    divider
    sleep 2
  else
    echo "Please remove the .deb package manually and re-run this script."
    exit 0
  fi
fi

install_deps() {
  if [[ "$DISTRO" == "fedora" || "$DISTRO" == "rhel" || \
        "$DISTRO" == "centos" || "$DISTRO" == "rocky" || \
        "$DISTRO" == "almalinux" ]]; then
    echo "Detected Red Hat-based system ($DISTRO), using dnf..."
    sudo dnf install -y \
      cmake ninja-build gcc-c++ perl python3 git wget file \
      openssl-devel fontconfig-devel freetype-devel \
      harfbuzz-devel libjpeg-turbo-devel libpng-devel \
      zlib-devel brotli-devel dbus-devel glib2-devel \
      at-spi2-core-devel mesa-libGL-devel mesa-libEGL-devel \
      mesa-libgbm-devel libdrm-devel libinput-devel \
      vulkan-loader-devel \
      libxkbcommon-devel libxkbcommon-x11-devel \
      xcb-util-devel xcb-util-image-devel xcb-util-keysyms-devel \
      xcb-util-renderutil-devel xcb-util-wm-devel xcb-util-cursor-devel \
      libXrender-devel libXi-devel \
      pulseaudio-libs-devel alsa-lib-devel \
      gstreamer1-devel gstreamer1-plugins-base-devel \
      wayland-devel wayland-protocols-devel

  elif [[ "$DISTRO" == "ubuntu" || "$DISTRO" == "debian" || \
          "$DISTRO" == "linuxmint" || "$DISTRO" == "pop" ]]; then
    echo "Detected Debian-based system ($DISTRO), using apt..."
    sudo apt-get update
    sudo apt-get install -y --ignore-missing \
      build-essential file wget git cmake ninja-build perl python3 \
      libssl-dev libfontconfig1-dev libfreetype-dev \
      libharfbuzz-dev libjpeg-dev libpng-dev \
      zlib1g-dev libbrotli-dev libdbus-1-dev libglib2.0-dev \
      libatspi2.0-dev libgl-dev libegl-dev libgbm-dev \
      libdrm-dev libinput-dev libvulkan-dev \
      mesa-utils libglu1-mesa-dev freeglut3-dev mesa-common-dev \
      libxkbcommon-dev libxkbcommon-x11-dev \
      libxcb-util-dev libxcb-image0-dev libxcb-keysyms1-dev \
      libxcb-render-util0-dev libxcb-icccm4-dev libxcb-cursor-dev \
      libxrender-dev libxi-dev \
      libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
      libwayland-dev wayland-protocols

    # Handle libasound rename in Ubuntu 24+
    sudo apt-get install -y libasound2-dev || sudo apt-get install -y libasound2t64

  elif [[ "$DISTRO" == "arch" || "$DISTRO" == "manjaro" || \
          "$DISTRO" == "endeavouros" || "$DISTRO" == "garuda" ]]; then
    echo "Detected Arch-based system ($DISTRO), using pacman..."
    sudo pacman -Syu --noconfirm \
      cmake ninja gcc perl python git wget \
      openssl fontconfig freetype2 \
      harfbuzz libjpeg-turbo libpng \
      zlib brotli dbus glib2 \
      at-spi2-core mesa libglvnd \
      libdrm libinput \
      vulkan-icd-loader \
      libxkbcommon \
      xcb-util xcb-util-image xcb-util-keysyms \
      xcb-util-renderutil xcb-util-wm xcb-util-cursor \
      libxrender libxi \
      libpulse alsa-lib \
      gst-plugins-base \
      wayland wayland-protocols \
      libxrandr libxext libxfixes \
      libxcb libxshmfence \
      libx11 \
      yasm nasm \
      clang llvm

  else
    echo "Unsupported distribution: $DISTRO"
    echo "Please install dependencies manually and re-run."
    exit 1
  fi
}

# --- Check for legacy ~/.local installation and offer to remove it ---
clear
divider
echo "Checking for legacy installation....."
if [ -f "${LEGACY_BIN}" ]; then
  divider
  echo -e "${red}A legacy JS8Call installation was found in ~/.local.

The new installer places JS8Call and its libraries in system paths
(/usr/bin and /usr/lib/js8call) for consistency with the .deb package.
The legacy installation should be removed.${clear_color}"
  divider
  read -p "Remove legacy ~/.local installation? Yes(y) / No(n): " REMOVE_LEGACY </dev/tty
  if [ "${REMOVE_LEGACY}" = "y" ]; then
    echo "Removing legacy JS8Call binary..."
    rm -f "${LEGACY_BIN}"
    echo "Removing legacy Qt 6.9.3..."
    rm -rf "${LEGACY_QT_DIR}"
    echo "Removing legacy libraries..."
    rm -rf "${LEGACY_LIB_DIR}"
    echo "Removing legacy desktop entry..."
    rm -f "${LEGACY_DESKTOP}"
    echo "Removing legacy icon..."
    rm -f "${LEGACY_ICON}"
    divider
    echo "Legacy installation removed."
    divider
    sleep 2
  fi
fi

# --- Check for existing system installation ---
divider
echo "Checking for existing installation....."
if [ -f "/usr/bin/JS8Call" ]; then
  divider
  echo "An existing JS8Call installation was found. Do you want to
uninstall it? Selecting No will overwrite it with the new version."
  read -p "Uninstall current JS8Call? Yes(y) / No(n): " UNINSTALL </dev/tty
  if [ "${UNINSTALL}" = "y" ]; then
    echo "Removing JS8Call binary..."
    sudo rm -f /usr/bin/JS8Call
    echo "Removing Qt 6.11.1..."
    sudo rm -rf "${JS8_QT_DIR}"
    echo "Removing libraries..."
    sudo rm -rf "${JS8_INSTALL_PREFIX}"
    echo "Removing desktop entry..."
    sudo rm -f /usr/share/applications/JS8Call.desktop
    echo "Removing icon..."
    sudo rm -f /usr/share/icons/hicolor/scalable/apps/js8call.svg
    # Remove ldconfig entry
    sudo rm -f /etc/ld.so.conf.d/js8call.conf
    sudo ldconfig
    divider
    echo "JS8Call has been uninstalled.
To reinstall, run this script again."
    divider
    exit 0
  fi
fi

# --- Introduction ---
clear
divider
echo -e "This script will fetch necessary sources and dependencies to build
JS8Call and install it on your system. It requires sudo access to install
to system paths (/usr/bin and /usr/lib/js8call).
If you already have an existing JS8Call installation from your distribution
you will be able to run either version, but not at the same time.
Please see the NOTES on the next page:"
divider
read -p "Press Enter to continue" </dev/tty

clear
echo "NOTES:
The newest versions of JS8Call require Qt v6.11.1 to run correctly. Most
linux distributions do not package Qt6.11.1. The JS8Call project provides
pre-compiled Qt6.11.1 libraries that this script will fetch and install to
/usr/lib/js8call/Qt. This will not affect whatever version of Qt you have
installed from your distribution. The two versions can co-exist and
JS8Call will be linked with the Qt6.11.1 installation only.

JS8Call and its libraries install to:
  Binary    : /usr/bin/JS8Call
  Libraries : /usr/lib/js8call
  Qt 6.11.1  : /usr/lib/js8call/Qt

This is consistent with the JS8Call .deb package. If you have previously
installed JS8Call via the .deb package this script will overwrite it.

Legacy versions of JS8Call (i.e. <=2.3.1) are named with a lower-case
convention (js8call). Versions 2.4.0 and later are named with an upper-case
convention (JS8Call v2.5.0 and later) or JS8Call-improved (v2.4.0). These
versions can co-exist on Linux."
divider
read -p "Press Enter to continue" </dev/tty

clear
echo "AUDIO:
Newer versions of Qt use the FFmpeg audio backend. ALSA audio is deprecated.
JS8Call 2.3 and later requires PipeWire or PulseAudio — there will be no audio
without one of these.

SETTINGS:
JS8Call stores its configuration in ~/.config/JS8Call.ini (versions 2.5.0
and later) and ~/.config/js8call.ini (versions 2.3.x and earlier). These
files can co-exist without conflict. If you are upgrading from 2.3.x your
old settings will NOT be automatically migrated — JS8Call will start with
default settings on first launch. You will need to reconfigure your
audio devices, callsign, and radio settings. Your old js8call.ini remains
untouched and will continue to be used if you run the legacy version."
divider
read -p "Press Enter to continue" </dev/tty

# --- Sudo warning for Debian users ---
if [[ "$DISTRO" == "debian" ]]; then
  clear
  echo -e "NOTE for Debian users:
Debian does not add users to the sudo group by default. If you have not
already done this, run the following as root, replacing
${red}your_username${clear_color} with your login name:

  ${red}/usr/sbin/usermod -aG sudo your_username${clear_color}

You will need to reboot for this change to take effect."
  divider
fi

read -p "Continue with installation? Yes(y) / No(n): " INSTALL </dev/tty
if [ ! "${INSTALL}" = "y" ]; then
  echo "Installation cancelled."
  exit 0
fi

# --- Install system dependencies ---
clear
echo "Installing build dependencies..."
install_deps

# --- Create development directory ---
mkdir -p "$HOME/development"

# --- Create system library directory owned by root ---
sudo mkdir -p "${JS8_INSTALL_PREFIX}"
sudo chown root:root "${JS8_INSTALL_PREFIX}"

# --- Detect architecture ---
clear
divider
JS8_ARCH=$(uname -m)
echo "System architecture: $JS8_ARCH"
divider
sleep 2

# --- Fetch Qt6 and js8lib if not already installed ---
echo "Checking for Qt 6.11.1..."
divider
sleep 2

cd "$HOME/development"

if [ ! -d "${JS8_QT_DIR}" ]; then
  if [ "${JS8_ARCH}" = "aarch64" ]; then
    wget -c https://github.com/JS8Call-improved/js8lib/releases/download/lib%2F4.0/Qt6.11.1_Linux_aarch64_pkg.tar.gz
    wget -c https://github.com/JS8Call-improved/js8lib/releases/download/lib%2F4.0/js8lib4.0-Linux_aarch64_pkg.tar.gz
    sudo tar -xzvf Qt6.11.1_Linux_aarch64_pkg.tar.gz -C "${JS8_INSTALL_PREFIX}"
    rm Qt6.11.1_Linux_aarch64_pkg.tar.gz
    sudo tar -xzvf js8lib4.0-Linux_aarch64_pkg.tar.gz -C "${JS8_INSTALL_PREFIX}" --strip-components=1
    rm js8lib4.0-Linux_aarch64_pkg.tar.gz
  else
    wget -c https://github.com/JS8Call-improved/js8lib/releases/download/lib%2F4.0/Qt6.11.1_Linux_x86_64_pkg.tar.gz
    wget -c https://github.com/JS8Call-improved/js8lib/releases/download/lib%2F4.0/js8lib4.0-Linux_x86_64_pkg.tar.gz
    sudo tar -xzvf Qt6.11.1_Linux_x86_64_pkg.tar.gz -C "${JS8_INSTALL_PREFIX}"
    rm Qt6.11.1_Linux_x86_64_pkg.tar.gz
    sudo tar -xzvf js8lib4.0-Linux_x86_64_pkg.tar.gz -C "${JS8_INSTALL_PREFIX}" --strip-components=1
    rm js8lib4.0-Linux_x86_64_pkg.tar.gz
  fi
  echo "Qt 6.11.1 and library archives extracted and removed."

  # Register private library paths with the system linker
  # This allows JS8Call to find its bundled libraries at runtime
  echo "${JS8_INSTALL_PREFIX}/lib" | sudo tee /etc/ld.so.conf.d/js8call.conf
  echo "${JS8_INSTALL_PREFIX}/Qt/lib" | sudo tee -a /etc/ld.so.conf.d/js8call.conf
  sudo ldconfig

else
  echo "Qt 6.11.1 already installed — skipping download."
  divider
  sleep 2
fi

# --- Fetch JS8Call source ---
clear
echo "Fetching JS8Call source code..."
divider

if [ ! -d "$HOME/development/JS8Call-improved" ]; then
  cd "$HOME/development"
  git clone "https://github.com/JS8Call-improved/JS8Call-improved.git"
  cd "$HOME/development/JS8Call-improved"
  git checkout "${JS8_VERSION}"
else
  echo "Source directory already exists — checking for updates..."
  cd "$HOME/development/JS8Call-improved"
  git fetch origin
  git checkout "${JS8_VERSION}"
  git pull origin "${JS8_VERSION}"
fi
sleep 2

# --- Build JS8Call ---
cd "$HOME/development/JS8Call-improved"
BRANCH=$(git branch --show-current)

clear
divider
echo "JS8Call Build Details:
  Qt version : 6.11.1 with FFmpeg audio (requires PulseAudio or PipeWire)
  Branch     : JS8Call-improved ${BRANCH}
  Distro     : ${DISTRO} / ${JS8_ARCH}"
divider
read -p "Press Enter to continue" </dev/tty

# Remove any previous build directory to avoid stale config
cd "$HOME/development/JS8Call-improved"
rm -rf "$HOME/development/JS8Call-improved/build"
mkdir "$HOME/development/JS8Call-improved/build"
cd "$HOME/development/JS8Call-improved/build"

cmake \
  -DCMAKE_PREFIX_PATH="${JS8_INSTALL_PREFIX};${JS8_QT_DIR}" \
  -DHAMLIB_ROOT="${JS8_INSTALL_PREFIX}" \
  ..
cmake --build . --parallel $(nproc)

# --- Install binary and desktop integration ---
sudo cp JS8Call /usr/bin/
sudo chmod 755 /usr/bin/JS8Call

# Desktop entry — uses system paths consistent with .deb install
sudo bash -c 'cat > /usr/share/applications/JS8Call.desktop << EOF
[Desktop Entry]
Type=Application
Name=JS8Call
Exec=/usr/bin/JS8Call
Icon=js8call
Terminal=false
Categories=HamRadio;Network;
EOF'

# Install icon to standard hicolor theme location
sudo mkdir -p /usr/share/icons/hicolor/scalable/apps
sudo cp ../artwork/icon_128.svg /usr/share/icons/hicolor/scalable/apps/js8call.svg

# Refresh desktop menu database
if command -v update-desktop-database > /dev/null 2>&1; then
  sudo update-desktop-database /usr/share/applications
fi

# --- Cleanup prompt ---
clear
divider
echo "DONE!"
echo "Do you want to remove the JS8Call development directory?
The program will run fine without it and you can re-fetch it with this
script at any time."
divider
read -p "Remove JS8Call source tree? Yes(y) / No(n): " CLEANUP </dev/tty

if [ "${CLEANUP}" = "y" ]; then
  rm -rf "$HOME/development"
  echo "Source tree removed."
else
  echo "Source tree kept at ~/development."
fi

divider
echo "JS8Call is installed. The launcher should appear in your application menu.
On KDE you may need to log out and back in to refresh the menu."
divider
exit 0
