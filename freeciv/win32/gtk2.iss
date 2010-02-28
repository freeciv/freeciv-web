[Setup]
AppName=Freeciv
AppVerName=Freeciv CVS HEAD 20030703
AppVersion=CVS HEAD 20030703
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
SourceDir=..
OutputDir=..
OutputBaseFilename=Freeciv-CVS
ChangesAssociations=no
; the line below skips Types section and always sets Type to custom
AlwaysShowComponentsList=yes
MinVersion=0,4.0

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
Source: "data\trident\*.png"; DestDir: "{app}\data\trident"; Components: client
Source: "data\trident\*.spec"; DestDir: "{app}\data\trident"; Components: client
Source: "data\isotrident\*.png"; DestDir: "{app}\data\isotrident"; Components: client
Source: "data\isotrident\*.spec"; DestDir: "{app}\data\isotrident"; Components: client
Source: "data\misc\*.png"; DestDir: "{app}\data\misc"; Components: client
Source: "data\misc\*.spec"; DestDir: "{app}\data\misc"; Components: client
Source: "data\trident.tilespec"; DestDir: "{app}\data"; DestName: "trident.tilespec"; Components: client
Source: "data\isotrident.tilespec"; DestDir: "{app}\data"; DestName: "isotrident.tilespec"; Components: client
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
Source: "win32\server.ico"; DestDir: "{app}\data\misc"; DestName: "server.ico"; Components: server
Source: "win32\client.ico"; DestDir: "{app}\data\misc"; DestName: "client.ico"; Components: client
;Source: "po\*.gmo"; DestDir: "{app}\po"; Components: lang

; Libraries
Source: "zlib.dll"; DestDir: "{app}"; Components: main
Source: "SDL_mixer.dll"; DestDir: "{app}"; Components: client
Source: "SDL.dll"; DestDir: "{app}"; Components: client
Source: "iconv.dll"; DestDir: "{app}"; Components: client
Source: "jpeg-62.dll"; DestDir: "{app}"; Components: client
Source: "libpng.dll"; DestDir: "{app}"; Components: client

[Icons]
Name: "{group}\Freeciv Client"; Filename: "{app}\civclient.exe"; Comment: "Play Freeciv!"; IconFilename: "{app}\data\misc\client.ico"; Components: client; WorkingDir: "{app}"
Name: "{group}\Freeciv Server"; Filename: "{app}\civserver.exe"; Comment: "Start a Freeciv server"; IconFilename: "{app}\data\misc\server.ico"; Components: server; WorkingDir: "{app}"
Name: "{group}\Uninstall Freeciv"; Filename: "{uninstallexe}";
Name: "{userdesktop}\Freeciv Client"; Filename: "{app}\civclient.exe"; Tasks: desktopicon; Comment: "Play Freeciv!"; IconFilename: "{app}\data\misc\client.ico"; Components: client; WorkingDir: "{app}"
Name: "{userdesktop}\Freeciv Server"; Filename: "{app}\civserver.exe"; Tasks: desktopicon; Comment: "Start a Freeciv server"; IconFilename: "{app}\data\misc\server.ico"; Components: server; WorkingDir: "{app}"

[Registry]
Root: HKLM; Subkey: "Software\Microsoft\Windows\CurrentVersion\App Paths\civclient.exe"; Flags: uninsdeletekeyifempty
Root: HKLM; Subkey: "Software\Microsoft\Windows\CurrentVersion\App Paths\civserver.exe"; Flags: uninsdeletekeyifempty
Root: HKLM; Subkey: "Software\Microsoft\Windows\CurrentVersion\App Paths\civclient.exe"; ValueType: string; ValueData: "{app}\civclient.exe"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "Software\Microsoft\Windows\CurrentVersion\App Paths\civserver.exe"; ValueType: string; ValueData: "{app}\civserver.exe"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "Software\Microsoft\Windows\CurrentVersion\App Paths\civclient.exe"; ValueType: string; ValueName: "Path"; ValueData: "{app};{code:GetGtkPath}\lib;{code:GetGtkPath}\bin"; Components: client; Flags: uninsdeletevalue
Root: HKLM; Subkey: "Software\Microsoft\Windows\CurrentVersion\App Paths\civserver.exe"; ValueType: string; ValueName: "Path"; ValueData: "{app};{code:GetGtkPath}\lib;{code:GetGtkPath}\bin"; Components: server; Flags: uninsdeletevalue

[Code]
var
  Exists: Boolean;
  GtkPath: String;

function GetGtkInstalled (): Boolean;
begin
  Exists := RegQueryStringValue (HKLM, 'Software\GTK\2.0', 'Path', GtkPath);
  if not Exists then begin
    Exists := RegQueryStringValue (HKCU, 'Software\GTK\2.0', 'Path', GtkPath);
  end;
   Result := Exists
end;

function GetGtkPath (S: String): String;
begin
    Result := GtkPath;
end;

function InitializeSetup(): Boolean;
begin
  Result := GetGtkInstalled ();
  if not Result then begin
    MsgBox ('Please install the GTK+ 2.0 Runtime Environment before installing Freeciv.  You can obtain GTK+ from http://www.dropline.net/gtk.', mbError, MB_OK);
  end;
end;

