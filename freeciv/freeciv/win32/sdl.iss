[Setup]
AppName=Freeciv
AppVerName=Freeciv CVS Head (EXPERIMENTAL WINDOWS SDL BUILD)
AppVersion=CVS Head
AppPublisher=The Freeciv Team
AppPublisherURL=http://www.freeciv.org
AppCopyright=Copyright (C) 2003 The Freeciv Team
AppSupportURL=http://www.freeciv.org
AppUpdatesURL=http://www.freeciv.org
DefaultDirName={pf}\Freeciv
DefaultGroupName=Freeciv
DisableStartupPrompt=yes
WindowShowCaption=yes
WindowVisible=yes
LicenseFile=COPYING
Compression=bzip/9
SourceDir=.
OutputDir=.
OutputBaseFilename=Freeciv-CVS
ChangesAssociations=no
; the line below skips Types section and always sets Type to custom
AlwaysShowComponentsList=yes
;MinVersion=0,4.0

[Types]
Name: "full"; Description: "Full installation"
Name: "clientonly"; Description: "Client only"
Name: "serveronly"; Description: "Server only"
Name: "custom"; Description: "Custom installation"; Flags: iscustom

[Components]
Name: "main"; Description: "Main Files"; Types: full custom serveronly clientonly; Flags: fixed
Name: "server"; Description: "Freeciv server"; Types: full custom serveronly
Name: "client"; Description: "Freeciv client"; Types: full custom clientonly
Name: "sound"; Description: "Sound pack"; Types: full custom clientonly
;Name: "lang"; Description: "Translations"; Types: full custom clientonly serveronly

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop icon"; GroupDescription: "Additional icons:"

[Dirs]
Name: "{app}\data"

[Files]
Source: "client\civclient.exe"; DestDir: "{app}"; DestName: "civclient.exe"; Components: client
Source: "server\civserver.exe"; DestDir: "{app}"; DestName: "civserver.exe"; Components: server
;Source: "data\trident\*.png"; DestDir: "{app}\data\trident"; Components: client
;Source: "data\trident\*.spec"; DestDir: "{app}\data\trident"; Components: client
Source: "data\misc\*.png"; DestDir: "{app}\data\misc"; Components: client
Source: "data\misc\*.spec"; DestDir: "{app}\data\misc"; Components: client
;Source: "data\trident.tilespec"; DestDir: "{app}\data"; DestName: "trident.tilespec"; Components: client
Source: "data\freeciv.rc-2.0"; DestDir: "{app}\data"; DestName: "freeciv.rc-2.0"; Components: client
Source: "data\helpdata.txt"; DestDir: "{app}\data"; DestName: "helpdata.txt"; Components: client
Source: "data\stdsounds.soundspec"; DestDir: "{app}\data"; DestName: "stdsounds.soundspec"; Components: sound
Source: "data\stdsounds\*.wav"; DestDir: "{app}\data\stdsounds"; Components: sound
Source: "data\civ1\*.ruleset"; DestDir: "{app}\data\civ1"; Components: server
Source: "data\civ2\*.ruleset"; DestDir: "{app}\data\civ2"; Components: server
Source: "data\default\*.ruleset"; DestDir: "{app}\data\default"; Components: server
Source: "data\history\*.ruleset"; DestDir: "{app}\data\history"; Components: server
Source: "data\nation\*.ruleset"; DestDir: "{app}\data\nation"; Components: server
Source: "data\scenario\*.sav"; DestDir: "{app}\data\scenario"; Components: server
Source: "COPYING"; DestDir: "{app}"; DestName: "COPYING.TXT"; Components: main
Source: "NEWS"; DestDir: "{app}"; DestName: "NEWS.TXT"; Components: main
Source: "doc\HOWTOPLAY"; DestDir: "{app}"; DestName: "HOWTOPLAY.TXT"; Components: main
Source: "doc\README"; DestDir: "{app}"; DestName: "README.TXT"; Components: main
Source: "doc\PEOPLE"; DestDir: "{app}"; DestName: "PEOPLE.TXT"; Components: main
Source: "data\misc\server.ico"; DestDir: "{app}\data\misc"; DestName: "server.ico"; Components: server
Source: "data\misc\client.ico"; DestDir: "{app}\data\misc"; DestName: "client.ico"; Components: client
;Source: "po\*.gmo"; DestDir: "{app}\po"; Components: lang

; Special SDL-Client data
Source: "data\deluxe.tilespec"; DestDir: "{app}\data"; Components: client
Source: "data\stdfont.ttf"; DestDir: "{app}\data"; Components: client
Source: "data\deluxe\*.spec"; DestDir: "{app}\data\deluxe"; Components: client
Source: "data\deluxe\*.png"; DestDir: "{app}\data\deluxe"; Components: client

; Special SDL-Client DLLs
Source: "jpeg-62.dll"; DestDir: "{app}"; Components: client
Source: "libpng.dll"; DestDir: "{app}"; Components: client
Source: "freetype-6.dll"; DestDir: "{app}"; Components: client
Source: "iconv.dll"; DestDir: "{app}"; Components: client
Source: "libpng.dll"; DestDir: "{app}"; Components: client
Source: "SDL.dll"; DestDir: "{app}"; Components: client
Source: "SDL_image.dll"; DestDir: "{app}"; Components: client
Source: "SDL_net.dll"; DestDir: "{app}"; Components: client
Source: "SDL_mixer.dll"; DestDir: "{app}"; Components: client
Source: "zlib.dll"; DestDir: "{app}"; Components: client

[Icons]
Name: "{group}\Freeciv Client"; Filename: "{app}\civclient.exe"; Parameters: "-t deluxe"; Comment: "Play Freeciv!"; IconFilename: "{app}\data\misc\client.ico"; Components: client; WorkingDir: "{app}"
Name: "{group}\Freeciv Server"; Filename: "{app}\civserver.exe"; Comment: "Start a Freeciv server"; IconFilename: "{app}\data\misc\server.ico"; Components: server; WorkingDir: "{app}"
Name: "{group}\Uninstall Freeciv"; Filename: "{uninstallexe}";
Name: "{userdesktop}\Freeciv Client"; Filename: "{app}\civclient.exe"; Parameters: "-t deluxe"; Tasks: desktopicon; Comment: "Play Freeciv!"; IconFilename: "{app}\data\misc\client.ico"; Components: client; WorkingDir: "{app}"
Name: "{userdesktop}\Freeciv Server"; Filename: "{app}\civserver.exe"; Tasks: desktopicon; Comment: "Start a Freeciv server"; IconFilename: "{app}\data\misc\server.ico"; Components: server; WorkingDir: "{app}"

[Registry]
Root: HKLM; Subkey: "Software\Microsoft\Windows\CurrentVersion\App Paths\civclient.exe"; Flags: uninsdeletekeyifempty
Root: HKLM; Subkey: "Software\Microsoft\Windows\CurrentVersion\App Paths\civserver.exe"; Flags: uninsdeletekeyifempty
Root: HKLM; Subkey: "Software\Microsoft\Windows\CurrentVersion\App Paths\civclient.exe"; ValueType: string; ValueData: "{app}\civclient.exe"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "Software\Microsoft\Windows\CurrentVersion\App Paths\civserver.exe"; ValueType: string; ValueData: "{app}\civserver.exe"; Flags: uninsdeletevalue

