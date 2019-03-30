@echo off
if not "%1" == "auto" set LANG=%1
set QT_PLUGIN_PATH=%~dp0\plugins
start "%~n0" /D . "%~dp0\freeciv-ruledit.exe" %2 %3 %4 %5 %6 %7 %8 %9
