#!/bin/bash

# (C) copyright 2025 Chris Olson AC9KH, Joseph Counsil K0OG. This script is for building and installing JS8Call-improved from source code.

# variables
red="\033[0;31m"
clear="\033[0m"
VERSION="master"

# functions
user_dialog() {
  echo "######################################################################"
}

error() {
  echo "Installation has exited ............"
  exit 1
}

clear

if [ ! "$(id -u)" -ne 0 ]; then
  user_dialog
  echo -e "${red}You are logged in as root.

You MUST be logged in as a normal user to run this script.

It is recommended to add your username to the sudo group. We need to install
system libraries that are required to build JS8Call-improved. This must be done as a
sudo user.

If you have not done this, this script will NOT RUN AS ROOT!${clear}"
  error
  user_dialog
  exit 1
fi

# check to see if there is an existing installation and ask to remove it
clear
user_dialog
echo "Checking for existing installation....."
if [ -e ${HOME}/.local/bin/JS8* ]; then
  user_dialog
  echo "We have found an existing JS8Call-improved installation. Do you want
  to uninstall it? If you select No(n) the newest version will be built and
  over-write the old one."
  read -p "Uninstall current JS8Call-improved? Yes(y) / No(n):-" UNINSTALL
  if [ ${UNINSTALL} = "y" ]; then
    echo "removing JS8Call-improved binary...."
    rm ~/.local/bin/JS8*
    echo "removing Qt 6.9.3....."
    rm -rf ~/.local/lib/Qt
    echo "removing JS8Call-improved.desktop......"
    rm ~/.local/share/applications/JS8*.desktop
    echo "removing JS8Call-improved icon......"
    rm ~/.local/share/icons/icon_128.svg
    echo "
    JS8Call-improved is uninstalled
    If you wish to re-install run this script again."
    user_dialog
    exit 1
  fi
fi

clear
user_dialog
echo -e "This script will fetch necessary sources and dependencies to build
JS8Call-improved and install it for the local user only on your system.
If you already have an existing JS8Call-improved installation from your distribution
or downloaded from the Releases it will not affect that and you will be able
run either version of the program, but not at the same time. Please see the 
NOTES on the next page:"
user_dialog

read -p "Press Enter to continue" </dev/tty
clear
echo "NOTES:
The newest versions of JS8Call-improved require Qt v6.9.3 to run correctly. Most
linux distributions do not package Qt6.9.3. The JS8Call-improved project has pre-compiled
Qt6.9.3 libraries by Chris-AC9KH that this script will fetch and install. This
will not affect whatever version of Qt you have installed from your distribution.
The two versions can co-exist and JS8Call-improved will be linked with the
Qt6.9.3 installation, which will be in your ~/.local/lib/Qt directory.

If you run this script a second time after installation, it will ask if you want
to uninstall JS8Call-improved. The installation of Qt6.9.3 will also be removed
during uninstall. But during build the Qt6.9.3 library archive that will be downloaded
will be saved in your Downloads folder. If you don't wish to keep it, delete it.

The JS8Call-improved menu item is tested with Gnome. It should
work on KDE since linux desktops are supposed to follow the conventions laid out
by freedesktop.org but this seems to be not always the case. If you don't get a
menu item on KDE et al under the Other category, the binary is installed to ~/.local/bin
where you can symlink it, place the symlink in a convenient location (like maybe the
desktop) and use it to launch the program. With KDE it seems you have to log out and log
back in to refresh the menus. With the standard Gnome desktops the menu item will appear
in the launcher automatically."
user_dialog

read -p "Press Enter to continue" </dev/tty
clear
echo "AUDIO:
Newer versions of Qt use FFmpeg audio backend. ALSA audio is deprecated. If you
are running an older version of JS8Call(2.2) it might be using ALSA backend. There will
be no audio on a Linux system with JS8Call-2.3 and later unless it is using PipeWire or
PulseAudio. When you start up JS8Call-improved it will use your current JS8Call settings
if you have an existing installation. There is some incompatibility with audio and text
encoding between JS8Call 2.2 (Fortran/Qt5) and 2.3 and newer (C++/Qt6).

The current JS8Call-improved codebase is coded for Qt6.9.3, developed on MacOS. It attempts
to be compatible with linux but we rely on linux users to provide feedback on issues that
may arise to identify linux-specific problems. This is our primary purpose in offering this
relatively easy method to install and run the latest code, with the proper Qt library, on linux.
Chris AC9KH, Joe K0OG"
user_dialog

read -p "Press Enter to continue" </dev/tty
clear
echo -e "The next step will install system dependencies as a sudo user. Since Fedora
systems do not add a user to sudo by default, and if you have not added yourself
to the sudo group, you must do so now by running (as root)

Copy and paste this command into your terminal if necessary and replace
${red}your_username${clear} with your login username:
${red}usermod -aG sudo your_username${clear}

With some systems you will have to log out and log back in, or reboot your machine for
your username to be added to sudo."
user_dialog

read -p "Continue with installation? Yes(y) / No(n):-" INSTALL

if [ ! "${INSTALL}" = "y" ]; then
  error
fi

clear
echo "installing build dependencies....."
sudo dnf -y install @c-development @development-tools
sudo dnf -y install file wget git cmake hamlib libhamlib* hamlib-devel
sudo dnf -y install mesa* libglu* freeglut* libusb1-devel libudev*
sudo dnf -y install libxkbcommon* libfftw3* fftw3* libvulkan*
sudo dnf -y install libboost*1.8* boost*1.8* boost*1.8* libxcb*
sudo dnf -y install mesa-libOpenCL*

if [ ! -d $HOME/development ]; then
  mkdir $HOME/development
else
  clear
  user_dialog
  echo "development directory already exists...."
  user_dialog
fi
sleep 3

echo "checking architecture......"
ARCH=$(arch)
echo "system architecture is $ARCH"
user_dialog
sleep 3

echo "checking for Qt6 version 6.9.3....."
user_dialog
sleep 3

# fetch Qt6 if doesn't already exist on system
cd ~/development
if [ ! -d $HOME/.local/lib/Qt ]; then
  mkdir ~/.local/lib
  if [ "${ARCH}" = "aarch64" ]; then
    wget https://github.com/JS8Call-improved/JS8Call-improved/releases/download/2.4/Qt6.9.3_Linux_aarch64.tar.gz
  else
    wget https://github.com/JS8Call-improved/JS8Call-improved/releases/download/2.4/Qt6.9.3_Linux_x86_64.tar.gz
  fi
else
  echo "~/.local/lib/Qt already exists......"
  user_dialog
fi
sleep 3
clear

# fetch JS8Call-improved source code from JS8Call-improved
echo "fetching JS8Call-improved source code....."
user_dialog
if [ ! -d ~/development/JS8Call-improved ]; then
  git clone "https://github.com/JS8Call-improved/JS8Call-improved.git"
  cd JS8Call-improved
  git checkout ${VERSION}
  cd ..
else
  echo "source code directory already exists!
  Checking for newer code......."
  cd JS8Call-improved
  git checkout ${VERSION}
  git pull
  cd ..
  user_dialog
fi
sleep 3

# prepare to build JS8Call-improved
if [ ! -d ~/.local/lib/Qt ]; then
  echo "extracting Qt6 to ~/.local/lib....."
  user_dialog
  sleep 3
  if [ "${ARCH}" = "aarch64" ]; then
    sudo tar -xzvf Qt6.9.3_Linux_aarch64.tar.gz -C ~/.local/lib/
    mv Qt6.9.3_Linux_aarch64.tar.gz ~/Downloads
    echo "Qt 6.9.3 archive has been moved to your Downloads folder"
  else
    sudo tar -xzvf Qt6.9.3_Linux_x86_64.tar.gz -C ~/.local/lib/
    echo "Qt 6.9.3 archive has been moved to your Downloads folder"
  fi
else
  echo "skipping installation of Qt6 - directory already exists...."
  user_dialog
  sleep 3
fi

clear
cd JS8Call-improved
BRANCH=$(git branch --show-current 2>&1)
user_dialog
echo "JS8Call-improved Build Details:
Qt version: 6.9.3 with FFmpeg audio. Requires PulseAudio or PipeWire
Branch: JS8Call-improved ${BRANCH}"
user_dialog
read -p "Press Enter to continue" </dev/tty

# remove potential build directory in case there is a bad configuration
# from a previous run on a different branch
rm -rf ~/development/JS8Call-improved/build

mkdir build
cd build
cmake -DCMAKE_PREFIX_PATH=~/.local/lib/Qt ..
cmake --build .

# install application for local user and create menu entry
if [ ! -d ~/.local/bin ]; then
  mkdir ~/.local/bin
fi
cp JS8Call ~/.local/bin
cp ../artwork/icon_128.svg ~/.local/share/icons/
touch ~/.local/share/applications/JS8Call.desktop
echo "[Desktop Entry]" >> ~/.local/share/applications/JS8Call.desktop
echo "Type=Application" >> ~/.local/share/applications/JS8Call.desktop
echo "Exec=${HOME}/.local/bin/JS8Call" >> ~/.local/share/applications/JS8Call.desktop
echo "Name=JS8Call" >> ~/.local/share/applications/JS8Call.desktop
echo "Icon=${HOME}/.local/share/icons/icon_128.svg" >> ~/.local/share/applications/JS8Call.desktop
echo "Terminal=false" >> ~/.local/share/applications/JS8Call.desktop

# ask if we're going to clean up the development directory and remove it
clear
user_dialog
echo "DONE!"
echo "Do you want to remove the JS8Call-improved development directory?
Don't worry the program will run fine without it and you can always
re-fetch it with this script later."
user_dialog
read -p "remove JS8Call-improved source tree? Yes(y) / No(n):-" CLEANUP
if [ "${CLEANUP}" = "y" ]; then
  rm -rf ~/development
  echo "JS8Call-improved source tree removed.
  install script has exited......."
  user_dialog
else
  echo "The development directory with the JS8Call-improved source tree has been \
  left on your system located at ~/development"
  user_dialog
  error
fi
exit 1

