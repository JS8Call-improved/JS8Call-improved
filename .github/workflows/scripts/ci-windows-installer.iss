#define MyAppName "JS8Call"
#define MyAppVersion "2.5.0"
#define MyAppPublisher "JS8Call-improved"
#define MyAppURL "https://www.js8call-improved.com/"
#define MyAppExeName "JS8Call.exe"

[Setup]
; NOTE: The value of AppId uniquely identifies this application. Do not use the same AppId value in installers for other applications.
AppId={{B5281957-28FD-4BAE-8D06-FC59898D850E}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
;AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
LicenseFile=D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call\LICENSE
DefaultDirName={autopf}\{#MyAppName}
UsePreviousAppDir=no
UninstallDisplayIcon={app}\{#MyAppExeName}
; "ArchitecturesAllowed=x64compatible" specifies that Setup cannot run
; on anything but x64 and Windows 11 on Arm.
ArchitecturesAllowed=x64compatible
; "ArchitecturesInstallIn64BitMode=x64compatible" requests that the
; install be done in "64-bit mode" on x64 or Windows 11 on Arm,
; meaning it should use the native 64-bit Program Files directory and
; the 64-bit view of the registry.
ArchitecturesInstallIn64BitMode=x64compatible
DisableProgramGroupPage=yes
; Uncomment the following line to run in non administrative install mode (install for current user only).
;PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
OutputDir=D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call
; This can be changed from the ci-windows.yml for release builds
OutputBaseFilename=JS8Call-installer
SolidCompression=yes
; WizardStyle introduced in 6.6.0, Github has 6.5.4
;WizardStyle=modern dynamic

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call\generic\*"; DestDir: "{app}\generic"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call\iconengines\*"; DestDir: "{app}\iconengines"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call\imageformats\*"; DestDir: "{app}\imageformats"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call\multimedia\*"; DestDir: "{app}\multimedia"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call\networkinformation\*"; DestDir: "{app}\networkinformation"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call\platforms\*"; DestDir: "{app}\platforms"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call\styles\*"; DestDir: "{app}\styles"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call\tls\*"; DestDir: "{app}\tls"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call\avcodec-61.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call\avformat-61.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call\avutil-59.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call\D3Dcompiler_47.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call\libfftw3f-3.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call\libgcc_s_seh-1.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call\libhamlib-4.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call\libstdc++-6.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call\libusb-1.0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call\libwinpthread-1.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call\opengl32sw.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call\Qt6Core.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call\Qt6Gui.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call\Qt6Multimedia.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call\Qt6Network.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call\Qt6SerialPort.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call\Qt6Svg.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call\Qt6Widgets.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call\swresample-5.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call\swscale-8.dll"; DestDir: "{app}"; Flags: ignoreversion
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
