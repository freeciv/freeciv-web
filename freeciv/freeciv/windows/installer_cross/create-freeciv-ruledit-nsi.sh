#!/bin/sh

# ./create-freeciv-ruledit.sh <Freeciv files directory> <version> <win32|win64|win>

cat <<EOF
; Freeciv Windows installer script
; some parts adapted from Wesnoth installer script

Unicode true
SetCompressor /SOLID lzma

!define APPNAME "Freeciv-ruledit"
!define VERSION $2
!define WIN_ARCH $3
!define APPID "\${APPNAME}-\${VERSION}"

!define MULTIUSER_EXECUTIONLEVEL Highest
!define MULTIUSER_MUI
!define MULTIUSER_INSTALLMODE_COMMANDLINE
!define MULTIUSER_INSTALLMODE_DEFAULT_REGISTRY_KEY "Software\\\${APPNAME}\\\${VERSION}"
!define MULTIUSER_INSTALLMODE_DEFAULT_REGISTRY_VALUENAME ""
!define MULTIUSER_INSTALLMODE_INSTDIR_REGISTRY_KEY "Software\\\${APPNAME}\\\${VERSION}"
!define MULTIUSER_INSTALLMODE_INSTDIR_REGISTRY_VALUENAME ""
!define MULTIUSER_INSTALLMODE_INSTDIR "\${APPNAME}-\${VERSION}"

!include "MultiUser.nsh"
!include "MUI2.nsh"
!include "nsDialogs.nsh"

;General

Name "Freeciv Ruleset Editor \${VERSION}"
OutFile "Output/\${APPNAME}-\${VERSION}-\${WIN_ARCH}-setup.exe"

;Variables

Var STARTMENU_FOLDER
Var DefaultLanguageCode
Var LangName

; Pages

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "$1\doc\freeciv\installer\COPYING.installer"
!insertmacro MUI_PAGE_COMPONENTS
Page custom DefaultLanguage DefaultLanguageLeave
!insertmacro MULTIUSER_PAGE_INSTALLMODE
!insertmacro MUI_PAGE_DIRECTORY

;Start Menu Folder Page Configuration
!define MUI_STARTMENUPAGE_REGISTRY_ROOT "SHCTX" 
!define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\\\${APPNAME}\\\${VERSION}"
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"
!define MUI_STARTMENUPAGE_DEFAULTFOLDER "\$(^Name)"

!insertmacro MUI_PAGE_STARTMENU "Application" \$STARTMENU_FOLDER
!insertmacro MUI_PAGE_INSTFILES

!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_FUNCTION RunFreecivRuledit
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

;Languages

!insertmacro MUI_LANGUAGE "English"

EOF

### required files ###

cat <<EOF
; The stuff to install
Section "\${APPNAME} (required)"

  SectionIn RO

  SetOutPath \$INSTDIR
EOF

  # find files and directories to exclude from default installation

  echo -n "  File /nonfatal /r "

  # languages
  echo -n "/x locale "

  # rulesets
  find $1/data -mindepth 1 -maxdepth 1 -name *.serv -printf %f\\n |
  sed 's|.serv||' |
  while read -r name
  do
  if test "x$name" != "xdefault" ; then
  echo -n "/x $name.serv /x $name "
  else
  echo -n "/x $name.serv "
  fi
  done

  echo "$1\\*.*"

cat <<EOF

  ; Write the installation path into the registry
  WriteRegStr "SHCTX" SOFTWARE\\\${APPNAME}\\\${VERSION} "" "\$INSTDIR"

  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
  CreateDirectory "\$SMPROGRAMS\\\$STARTMENU_FOLDER"
  CreateShortCut "\$SMPROGRAMS\\\$STARTMENU_FOLDER\Freeciv Ruleset Editor.lnk" "\$INSTDIR\freeciv-ruledit.cmd" "\$DefaultLanguageCode" "\$INSTDIR\freeciv-ruledit.exe" 0 SW_SHOWMINIMIZED
EOF

cat <<EOF

  CreateShortCut "\$SMPROGRAMS\\\$STARTMENU_FOLDER\Uninstall.lnk" "\$INSTDIR\uninstall.exe"
  CreateShortCut "\$SMPROGRAMS\\\$STARTMENU_FOLDER\Website.lnk" "\$INSTDIR\Freeciv.url"
  !insertmacro MUI_STARTMENU_WRITE_END

  ; Write the uninstall keys for Windows
  WriteRegStr "SHCTX" "Software\Microsoft\Windows\CurrentVersion\Uninstall\\\${APPID}" "DisplayName" "\$(^Name)"
  WriteRegStr "SHCTX" "Software\Microsoft\Windows\CurrentVersion\Uninstall\\\${APPID}" "UninstallString" '"\$INSTDIR\uninstall.exe"'
  WriteRegDWORD "SHCTX" "Software\Microsoft\Windows\CurrentVersion\Uninstall\\\${APPID}" "NoModify" 1
  WriteRegDWORD "SHCTX" "Software\Microsoft\Windows\CurrentVersion\Uninstall\\\${APPID}" "NoRepair" 1
  WriteUninstaller "uninstall.exe"

  SetOutPath \$INSTDIR
SectionEnd

EOF

### rulesets ###

cat <<EOF
SectionGroup "Rulesets"

EOF

find $1/data -mindepth 1 -maxdepth 1 -name *.serv -printf %f\\n |
sort |
sed 's|.serv||' |
while read -r name
do
if test -d $1/data/$name; then
# 'default' directory is not optional
if test "x$name" != "xdefault" ; then
echo "  Section \"$name\""
# Intentionally left .serv out. Ruledit does not use it.
# This script does use it, though, to detect rulesets.
echo "  SetOutPath \$INSTDIR/data/$name" | sed 's,/,\\,g'
echo "  File /r $1/data/$name/*.*"
echo "  SetOutPath \$INSTDIR"
echo "  SectionEnd"
echo
fi
fi
done

cat <<EOF
SectionGroupEnd

EOF

### additional languages ###

cat <<EOF
SectionGroup "Additional languages (translation %)"

EOF

cat ../../bootstrap/langstat_ruledit.txt |
sort -k 1 |
while read -r code prct name
do
if test -e $1/share/locale/$code/LC_MESSAGES/freeciv-ruledit.mo; then
echo "  Section \"$name ($code) $prct\""
echo "  SetOutPath \$INSTDIR/share/locale/$code" | sed 's,/,\\,g'
echo "  File /r $1/share/locale/$code/*.*"
echo "  SetOutPath \$INSTDIR"
echo "  SectionEnd"
echo
fi
done

cat <<EOF
SectionGroupEnd

EOF

cat <<EOF
;--------------------------------
;Installer Functions

Function .onInit

  !insertmacro MULTIUSER_INIT

FunctionEnd

Var DefaultLanguageDialog
Var DefaultLanguageLabel
Var DefaultLanguageDropList

Function DefaultLanguage
  !insertmacro MUI_HEADER_TEXT "Choose Default Language" ""

  nsDialogs::Create 1018
  Pop \$DefaultLanguageDialog

  \${If} \$DefaultLanguageDialog == error
    Abort
  \${EndIf}

  \${NSD_CreateLabel} 0 0 100% 30% \
"If you want to run Freeciv Ruleset Editor in a language other than your Windows language or \
if Freeciv's auto-detection of your Windows language does not work correctly, \
you can select a specific language to be used by Freeciv Ruleset Editor here. Be sure \
you haven't unmarked the installation of the corresponding language files \
in the previous dialog. You can also change this setting later in the Freeciv Ruleset Editor \
Start Menu shortcut properties."
  Pop \$DefaultLanguageLabel

  \${NSD_CreateDropList} 0 -60% 100% 13u ""
  Pop \$DefaultLanguageDropList

  \${NSD_CB_AddString} \$DefaultLanguageDropList "Autodetected"
  \${NSD_CB_SelectString} \$DefaultLanguageDropList "Autodetected"
  \${NSD_CB_AddString} \$DefaultLanguageDropList "US English (en_US)"
EOF

  cat ../../bootstrap/langstat_ruledit.txt |
  sort -k 1 |
  while read -r code prct name
  do
  if test -e $1/share/locale/$code/LC_MESSAGES/freeciv-ruledit.mo; then
  echo "  \${NSD_CB_AddString} \$DefaultLanguageDropList \"$name ($code) $prct\""
  fi
  done

cat <<EOF
  nsDialogs::Show
FunctionEnd

Function DefaultLanguageLeave
  \${NSD_GetText} \$DefaultLanguageDropList \$LangName
EOF

  echo "  \${If} \$LangName == \"Autodetected\""
  echo "    StrCpy \$DefaultLanguageCode \"auto\""
  echo "  \${EndIf}"
  echo "  \${If} \$LangName == \"US English (en_US)\""
  echo "    StrCpy \$DefaultLanguageCode \"en_US\""
  echo "  \${EndIf}"

  cat ../../bootstrap/langstat_ruledit.txt |
  while read -r code prct name
  do
    echo "  \${If} \$LangName == \"$name ($code) $prct\""
    echo "    StrCpy \$DefaultLanguageCode \"$code\""
    echo "  \${EndIf}"
  done

cat <<EOF
FunctionEnd

Function RunFreecivRuledit
  nsExec::Exec '"\$INSTDIR\freeciv-ruledit.cmd" \$DefaultLanguageCode'
FunctionEnd

EOF

### uninstall section ###

cat <<EOF
; special uninstall section.
Section "Uninstall"

  ; remove files
EOF

find $1 -type f |
grep -v '/$' |
sed 's|[^/]*||' |
while read -r name
do
echo "  Delete \"\$INSTDIR$name\"" | sed 's,/,\\,g'
done

find $1 -depth -type d |
grep -v '/$' |
sed 's|[^/]*||' |
while read -r name
do
echo "  RMDir \"\$INSTDIR$name\"" | sed 's,/,\\,g'
done

cat <<EOF

  ; MUST REMOVE UNINSTALLER, too
  Delete "\$INSTDIR\uninstall.exe"

  ; remove install directory, if empty
  RMDir "\$INSTDIR"

  ; remove shortcuts, if any.
  !insertmacro MUI_STARTMENU_GETFOLDER "Application" \$STARTMENU_FOLDER
  Delete "\$SMPROGRAMS\\\$STARTMENU_FOLDER\*.*"
  RMDir "\$SMPROGRAMS\\\$STARTMENU_FOLDER"

  ; remove registry keys
  DeleteRegKey "SHCTX" "Software\Microsoft\Windows\CurrentVersion\Uninstall\\\${APPID}"
  DeleteRegKey /ifempty "SHCTX" SOFTWARE\\\${APPNAME}\\\${VERSION}
  DeleteRegKey /ifempty "SHCTX" SOFTWARE\\\${APPNAME}
SectionEnd
EOF

cat <<EOF
;--------------------------------
;Uninstaller Functions

Function un.onInit

  !insertmacro MULTIUSER_UNINIT

FunctionEnd

EOF
