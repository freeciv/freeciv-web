@echo off
if %1 NEQ auto set LANG=%1
start freeciv-server.exe %2 %3 %4 %5 %6 %7 %8 %9
