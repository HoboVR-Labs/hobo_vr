@echo off
IF EXIST "C:\Program Files (x86)\Steam\steamapps\common\SteamVR\bin\win32\" (
	SET "var=%~dp0"

	"C:\Program Files (x86)\Steam\steamapps\common\SteamVR\bin\win32\vrpathreg.exe" removedriver %var%
	"C:\Program Files (x86)\Steam\steamapps\common\SteamVR\bin\win32\vrpathreg.exe" show

) ELSE (
	echo "SteamVR not located in C:\\Program Files (x86)\\Steam\\steamapps\\common\\SteamVR - Installation Failed"
)
pause
