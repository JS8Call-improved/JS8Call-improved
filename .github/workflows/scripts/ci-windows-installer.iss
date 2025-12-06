#define MyAppName "JS8Call"
#define MyAppVersion "2.5.0"
#define MyAppPublisher "JS8Call-improved"
#define MyAppURL "https://www.js8call-improved.com/"
#define MyAppExeName "JS8Call.exe"

[Setup]
; NOTE: The value of AppId uniquely identifies this application. Do not use the same AppId value in installers for other applications.
AppId={{5B3F1070-8CE9-41A5-8F93-E5445849F7BC}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
;AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
LicenseFile=D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call-improved\LICENSE
DefaultDirName={autopf}\{#MyAppName}
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
OutputDir=D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call-improved
; This can be changed from the ci-windows.yml for release builds
OutputBaseFilename=JS8Call-improved-installer-250
SolidCompression=yes
; WizardStyle introduced in 6.6.0, Github has 6.5.4
;WizardStyle=modern dynamic

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call-improved\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call-improved\generic\*"; DestDir: "{app}\generic"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call-improved\iconengines\*"; DestDir: "{app}\iconengines"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call-improved\imageformats\*"; DestDir: "{app}\imageformats"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call-improved\multimedia\*"; DestDir: "{app}\multimedia"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call-improved\networkinformation\*"; DestDir: "{app}\networkinformation"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call-improved\platforms\*"; DestDir: "{app}\platforms"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call-improved\styles\*"; DestDir: "{app}\styles"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call-improved\tls\*"; DestDir: "{app}\tls"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call-improved\avcodec-61.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call-improved\avformat-61.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call-improved\avutil-59.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call-improved\D3Dcompiler_47.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call-improved\libfftw3f-3.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call-improved\libgcc_s_seh-1.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call-improved\libhamlib-4.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call-improved\libstdc++-6.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call-improved\libusb-1.0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call-improved\libwinpthread-1.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call-improved\opengl32sw.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call-improved\Qt6Core.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call-improved\Qt6Gui.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call-improved\Qt6Multimedia.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call-improved\Qt6Network.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call-improved\Qt6SerialPort.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call-improved\Qt6Svg.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call-improved\Qt6Widgets.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call-improved\swresample-5.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "D:\a\JS8Call-improved\JS8Call-improved\build\JS8Call-improved\swscale-8.dll"; DestDir: "{app}"; Flags: ignoreversion
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
