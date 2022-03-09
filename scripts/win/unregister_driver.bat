@echo off
rem IF EXIST "C:\Program Files (x86)\Steam\steamapps\common\SteamVR\bin\win32\" (
rem IF EXIST "%~dp0..\..\hobovr" (
rem 	set old_pwd="%~dp0"
rem 	cd  "..\..\hobovr"

rem rem fuck windows, this shit is disgusting, you need a fucking for loop to capture an output of a command wtf
rem 	FOR /F "tokens=* USEBACKQ" %%F IN (`cd`) DO (
rem 	SET var=%%F
rem 	)
rem 	cd %old_pwd%
rem 	"C:\Program Files (x86)\Steam\steamapps\common\SteamVR\bin\win32\vrpathreg.exe" removedriver "%var%"
rem 	"C:\Program Files (x86)\Steam\steamapps\common\SteamVR\bin\win32\vrpathreg.exe" show
rem ) ELSE (
rem 	echo "could not find driver, is the repository broken?"
rem )
rem ) ELSE (
rem echo "SteamVR not located in C:\\Program Files (x86)\\Steam\\steamapps\\common\\SteamVR - Uninstallation Failed"
rem )
echo "this script is not finished..."
pause
