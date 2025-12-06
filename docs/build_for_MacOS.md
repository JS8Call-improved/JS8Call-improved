
------------------------------------------------------------------------------
# MacOS Prerequisites:
You will need Xcode commandline tools installed. Xcode can be downloaded from the Apple Store for your Mac, or you can install just the command line tools by opening Terminal and typing xcode-select --install. For this example I used Xcode 16.2 on MacOS 15 Sequoia

Obtain cmake v4.03 from https://github.com/Kitware/CMake/releases/download/v4.0.3/cmake-4.0.3.tar.gz Unpack the cmake source archive with Finder and follow the instructions in the README.rst to build and install cmake on MacOS. This will be in Terminal by using cd to change into the cmake-4.0.3 directory and running the following command:
```
./bootstrap && make && sudo make install
```
NOTE: you can also install cmake with homebrew. Check the homebrew documentation on how to get homebrew and do this, if desired.

------------------------------------------------------------------------------
# Getting the Libraries on MacOS
Below are the required libraries and tested versions for a JS8Call-improved build on MacOS along with links to download them.

* [libusb-1.0.29](https://github.com/libusb/libusb/releases/download/v1.0.29/libusb-1.0.29.tar.bz2)

* [Hamlib-4.6.4](https://github.com/Hamlib/Hamlib/releases/download/4.6.4/hamlib-4.6.4.tar.gz)

* [fftw-3.3.10](https://fftw.org/fftw-3.3.10.tar.gz)

* [boost_1_88_0](https://archives.boost.io/release/1.88.0/source/boost_1_88_0.tar.gz)

* Qt 6.9.3 - this one is non-trivial and is a monster. Failed builds are common and will likely discourage you from trying a JS8Call build. Recommendations below......

*   There is two ways to obtain the libraries and frameworks to compile JS8Call-improved; either build them yourself or fetch the pre-built libraries. Since building the libraries is non-trivial with Qt6 I recommend fetching the pre-built libraries from [here](https://github.com/JS8Call-improved/JS8Call-improved/releases/tag/2.4).

    If you wish to build the libraries yourself you can clone this [repository](https://github.com/JS8Call-improved/js8lib) and follow the developer instructions. You can check out one of the MacOS branches with `git switch release/branch-name` and follow the instructions to build your libraries. It's optional to build Qt with the developer library. I recommend building the base libraries and obtaining Qt 6.9.3 with the Online Installer from the Qt Group. I can't provide a download link for this because it requires that you create a free account with qt.io and review the licensing terms. Once you have an account you can download the online installer in your account downloads on qt.io

*   In Terminal create the directory structure to build JS8Call-improved with the following command.
    ```
    mkdir ~/development && mkdir ~/development/JS8Call
    ```
*   Download the library for your architecture with the above link and drag it to the project root `~development/JS8Call` in Finder. Double click on the archive to unpack it. It will create a folder called `js8lib`.

------------------------------------------------------------------------------
# Building JS8Call-improved on MacOS
We'll now fetch the JS8Call-improved sourcecode with git:
```
cd ~/development/JS8Call && git clone https://github.com/JS8Call-improved/JS8Call-improved.git src
```
Your libraries are now in `~/development/JS8Call/js8lib` and the JS8Call source code is in `~/development/JS8Call/src`. You need to copy the library folder to /usr/local/ before your proceed. To do this at the command prompt cd `~/development/JS8Call` and use
```
sudo cp -r js8lib /usr/local/
```
If you obtain Qt using the online installer it is recommended to install it in `~/development/JS8Call/Qt` and use Qt 6.9.3. Other versions of Qt may cause audio issues or inject other undesireable bugs. To prevent issues with missing libraries I recommend selecting the checkbox for Qt 6.9.3 and download everything for it. After JS8Call is built you don't need to keep this library on your system and it can be removed with the Qt Maintenance Tool which is found inside the Qt folder.

NOTE: the master (dev) branch will be checked out by default. If you want to check out a different branch you can use `cd ~/development/JS8Call/src` and then run `git switch release/2.4.0` to switch to and build the 2.4.0 branch, for instance. Or to list all the branches you can use `git branch -a`. It is now a simple matter of creating a build directory and run `cmake` to configure the build, followed by `cmake --build .`. You can copy and paste this command in Terminal if you like.
```
cd ~/development/JS8Call/src && mkdir build && cd build \
&& cmake -DCMAKE_PREFIX_PATH="/usr/local/js8lib;~/development/JS8Call/Qt" -DCMAKE_BUILD_TYPE=Release .. \
&& cmake --build .
```
An issue you may run into is that MacOS apps do not request permissions to use audio input correctly unless the app and libraries are signed with an Apple developer certificate. There is really no way around this. If your build keeps continually asking for permissions to use audio input you can create a free signing certificate that will allow the app to run on your local computer, or other computers with the same AppleID. This free signing certificate can be created from within Xcode if you have an AppleID that uses an icloud email account. Consult Apple's developer documentation [here](https://support.apple.com/guide/keychain-access/create-self-signed-certificates-kyca8916/mac) on how to do this.

If all goes well, you should end up with a `JS8Call.app` application in the build directory. Test by typing `open ./JS8Call.app`. Once you're satisfied with the test   results, drag JS8Call.app to /Applications.
