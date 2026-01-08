------------------------------------------------------------------------------
# Building JS8Call-improved on Windows

This is a general overview with some slight variations for building Qt6-compliant JS8Call-improved using JTSDK64-Tools (HamlibSDK) - see the detailed instructions for JTSDK64-Tools at Sourceforge:  https://hamlib-sdk.sourceforge.io/

**Prerequisites:**
Windows 10 or 11

**Set up the JTSDK64-Tools build environment**
1) Install the JTSDK (Hamlib-SDK) 4.1.0 (the latest version from https://sourceforge.net/projects/hamlib-sdk as of this writing)
2) In the ``C:\JTSDK64-Tools\config`` directory, set the Hamlib file marker name to "hlnone", the qt file marker name to "qt6.6.3", and the source file marker name to "src-none"
3) Edit the Versions.ini file to change the following lines thusly:
   ```
   boostv=1.88.0
   pulllatest=No
   cpuusage=All
   qt6v=6.9.3
   ```
   NOTE:  As of this writing, Qt6.6.3 is *required* for OmniRig functionality to work (newer Qt versions will not work with OmniRig), however we no longer recommend enabling OmniRig as it presents an apparent security risk for Windows.

4) Run the JTSDK64-Setup script to install all the components per the instructions and install Qt6.9.3, then close the powershell window
5) Run the JTSDK64-Tools, and run `Deploy-Boost` to install Boost 1.88.0 (may take a while!), then close the powershell window
6) Open JTSDK64-Tools, run mingw64 and `menu` and build Dynamic Hamlib (the default latest will install if you make no changes). To control the version installed, interrupt the Hamlib build process with a CTRL-C and open the Hamlib src directory (e.g., using Git Bash in Windows) and `git checkout 4.6.5` to set the version in source.  To see what versions of Hamlib are available, while in the hamlib directory, type `git branch -r` or `git tag` to get a list.  If you interrupted the build to select a specific Hamlib version, run mingw64 again then type `menu` and proceed to build Dynamic Hamlib.

**Building JS8Call**
1) Obtain the JS8Call source and place it in a folder named `wsjtx` in `C:\JTSDK64-Tools\tmp`, or open a Git Bash window in the `C:\JTSDK64-Tools\tmp` directory and issue a `git clone https://github.com/JS8Call-improved/JS8Call-improved.git wsjtx` command.  As with Hamlib, you can change the version of JS8Call you wish to build by opening a Git Bash command-line window in the wsjtx folder and issue the `git checkout <version>` command.  To see what versions are available in the repository, issue `git branch -r`
2) After Hamlib is built, close the powershell window, then re-open JTSDK64-Tools and build JS8Call with the `jtbuild package` command
3) Unless some requirements of the source change (e.g., a requirement to install a newer version of Qt or Boost), this build system can be used to simply do `jtbuild package` after the source has been updated.
   
   NOTE:  The JTSDK64-Tools build system will not package JS8Call.  There will be packaging errors presented when the build is finished.  IGNORE those errors (for now, with JTSDK64-Tools 4.1.0).  A future version of JTSDK64-Tools should include an option to "jtbuild noinstall" which will not present packaging errors.
5) If you wish to package JS8Call, we suggest that you install the Inno Installer from:  https://github.com/jrsoftware/issrc.  Detail of how to setup and develop an Inno installer script is beyond the scope of this document.  However, when setting up an installer, we have found that you must pull four .dll files into the JS8Call bin directory as follows:
   ```
   libfftw3f-3.dll from C:\JTSDK64-Tools\tools\fftw\3.3.10
   libhamlib-4.dll, libusb-1.0.dll, and libwinpthread-1.dll from C:\JTSDK64-Tools\tools\hamlib\bin
   ```
6) To run JS8Call from the build directory, the above-mentioned four .dll files will need to be copied into the binary directory (e.g., `C:\JTSDK64-Tools\tmp\wsjtx-output\qt\6.9.3\2.5.0\Release\build\JS8Call`), and the JS8Call.exe file in the `C:\JTSDK64-Tools\tmp\wsjtx-output\qt\6.9.3\2.5.0\Release\build` directory will need to be copied into the `C:\JTSDK64-Tools\tmp\wsjtx-output\qt\6.9.3\2.5.0\Release\build\JS8Call` directory.  JS8Call.exe in the `C:\JTSDK64-Tools\tmp\wsjtx-output\qt\6.9.3\2.6.0\Release\build\JS8Call` directory can now be executed.
------------------------------------------------------------------------------
# Building JS8Call-improved 2.4 and later with Qt Creator

**Set up the build environment**
1) Download and install CMake 4.1.1 from Kitware. You can search for this on the web.
2) Download and install git for Windows from GitHub. Again, you can search for this on the web.
3) Download and install the Qt Online Installer from qt.io. To do this you must create a free account with Qt starting at this link `https://doc.qt.io/qt-6/qt-online-installation.html` log into your new account and you will be able to download it.
4) After installation of Qt use the Qt Maintenance Tool to install Qt 6.9.3

**Notes On Installation of Qt6**
- Recommended location for your Qt install is `C:\Qt`
- During installation of Qt, select under Additional Libraries to get Qt Image Formats, Qt Multimedia, Qt Network Authorization and Qt Serial Port
- Select to install MinGW 13, and under Build Tools make sure MinGW 13 is checked. At the bottom of the list make sure to get Qt Creator.

**Files, Directories and Libraries**

* Create a directory called `development` at `C:\development`

* Download the Windows JS8Call-improved development library [here](https://github.com/JS8Call-improved/JS8Call-improved/releases/tag/2.4) and unzip the library inside of the development folder. This will create a folder called `js8lib` which contains the necessary development libraries to build JS8Call-improved

* Next we need to get the JS8Call-improved source code with git. This requires use of the Windows Command shell (installing the Windows PowerShell is recommended). Change to the C:\development folder and type `git clone https://github.com/JS8Call-improved/JS8Call-improved.git` This will create a folder called JS8Call-improved which contains the source code.

**Setting up Qt Creator**

* To get started with a CMake project in Qt Creator, open the program and select Open Project. Simply navigate to the CMakeLists.txt in `C:\development\JS8Call-improved` and select it. When the project window opens your available “kits” will be listed on the left side. Simply click the + button by the kits you want to use for this project (6.9.3). Qt Creator will create what is called an Initial Configuration. You can select to hide the inactive kits if you installed more than one version of Qt.

* You can select Manage Kits at the top left to make sure you have a valid compiler (which should be MinGW) for each one. The default build will be Debug. Under Build Settings select Add, and add a Release configuration and name it Release.

* Qt Creator automatically creates a build folder in the source tree. Inside the build folder Creator makes folders for each kit and build type. DO NOT delete these. They contain your project build configuration settings for each build type. At present Creator will say there’s no configuration for Release found. Don’t worry about that. We’re going to fix it in the next step where we move to the CMake keys.

* In the CMake configure settings pane (the center one) switch from Initial Configuration tab to Current Configuration. Then make the following changes……
- Change CMAKE_GENERATOR to MinGW Makefiles
- Change CMAKE_INSTALL_PREFIX to Program Files (not x86)
- Replace the entire CMAKE_PREFIX_PATH with the following
```
%{Qt:QT_INSTALL_PREFIX};C:\development\js8lib\lib64-msvc-14.3\cmake\Boost-1.89.0;C:\development\js8lib;C:\development\js8lib\bin
```
-  Click on the Run CMake button just below the CMake Settings and you should get a successful configuration for this kit

**Some information on this to better understand Qt Creator**

The Initial Configuration is what the raw CMake run generates. The Current Configuration is your custom configs with the prefix for the libraries and final build settings. It is what will be actually built. But we need to add two more steps to the build yet.
1) Move down to Build Steps and click on Add Build Step -> Custom Process Step. In the Command enter `cmd.exe`  Remove the entry from the Working directory and leave it blank. Then copy and paste this to the Arguments box
```
cd C:\development\JS8Call-improved\build\Desktop_Qt_6_8_3_MinGW_64_bit-Release && move JS8Call.exe .\JS8Call
```
2) Add another Build Step and in the Command box enter `cmd.exe` then copy and paste this to the Arguments box
```
cd C:\development\JS8Call-improved\build\Desktop_Qt_6_8_3_MinGW_64_bit-Release && copy C:\development\js8lib\dll\*.dll .\JS8Call
```
You can now click on the hammer in the lower left and the JS8Call-improved project should build. After the build completes you will find a folder inside the build -> (kit name) directory called JS8Call-improved. It will contain all the libraries the program needs to run, along with the JS8Call-improved executable. At this point you can use a Windows Installer package creator like NSIS or Inno Setup to create a Windows installer if you wish. Or if you are building only for your local computer you can move the JS8Call folder to C:\Program Files, create a shortcut to the JS8Call.exe executable, and place the shortcut on your desktop to launch the program.

If you wish to do another build later simply go to Build in the menu and select Clean Build Folder and it will remove all old build artifacts so you can do another build without reconfiguration of Qt Creator. The program will save your build setup as long as you don't delete the folders inside the `build` folder. But during a Clean Build Folder it will not remove the built JS8Call product folder. That must be deleted manually, or moved, before doing another build.

Note that this can not be a complete tutorial on how to use Qt Creator, only a general guide as to what is required to build JS8Call-improved 2.4 or later.
