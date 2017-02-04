; also in the code
#define version "0.x.x"

#define sourceFolder "..\..\namecore-release"
#define progname "NamecoinCore"

; this is necessary so that the link name is the same as in the client
#define autostartname "Namecoin"

#define exefile "namecoin-qt.exe"

[Messages]
WelcomeLabel2=%nThis will install [name/ver] on your computer.%n
ClickFinish=ATTENTION:  Please note it may take a couple of hours for the initial blockchain download to complete.%n%nClick Finish to exit Setup.
ConfirmUninstall=Are you sure you want to completely remove %1 and all of its components?%n%nThis will leave blockchain data and your wallet.dat file alone.
UninstalledAll=%1 was successfully removed from your computer.%n%nThere might still be blockchain data and your wallet.dat file in %APPDATA%\Namecoin

[Setup]
AppVerName={#progname} {#version}
AppName={#progname}
DefaultDirName={pf}\{#progname}
DefaultGroupName={#progname}
UninstallDisplayIcon={uninstallexe}
Compression=lzma2/ultra
SolidCompression=yes
OutputDir={#sourceFolder}
OutputBaseFilename={#progname}_{#version}_setup

[InstallDelete]
; clean up any autostart links for a fresh start
Type: files; Name: "{commonstartup}\{#autostartname}.lnk"
Type: files; Name: "{userstartup}\{#autostartname}.lnk"

[Files]
Source: "{#sourceFolder}\namecoin-qt.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#sourceFolder}\namecoind.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#sourceFolder}\namecoin-cli.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#sourceFolder}\namecoin-tx.exe"; DestDir: "{app}"; Flags: ignoreversion

[Tasks]
Name: startup; Description: "Automatically &start on login (current user)"; GroupDescription: "Autostart:";  Flags: unchecked;
Name: desktopicon; Description: "Create a &desktop icon"; GroupDescription: "Additional icons:"
Name: desktopicon\common; Description: "For all users"; GroupDescription: "Additional icons:"; Flags: exclusive unchecked;
Name: desktopicon\user; Description: "For the current user only"; GroupDescription: "Additional icons:"; Flags: exclusive;
Name: quicklaunchicon; Description: "Create a &Quick Launch icon"; GroupDescription: "Additional icons:"; Flags: unchecked;

[Icons]
Name: "{group}\{#progname}"; Filename: "{app}\{#exefile}"
Name: "{commondesktop}\Namecoin-Qt"; Filename: "{app}\{#exefile}"; Tasks: desktopicon\common
Name: "{userdesktop}\Namecoin-Qt"; Filename: "{app}\{#exefile}"; Tasks: desktopicon\user
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\Namecoin-Qt"; Filename: "{app}\{#exefile}"; Tasks: quicklaunchicon
Name: "{group}\uninstall"; Filename: "{uninstallexe}";
Name: "{userstartup}\{#autostartname}"; Filename: "{app}\{#exefile}"; Tasks: startup

[Run]
Filename: "{app}\{#exefile}"; Description: "Launch application"; Flags: postinstall skipifsilent nowait
