# conda execute
# env:
#  - nsis 3.*
#  - AccessControl
#  - InetC
#  - Nsisunz
#  - ExecDos
#  - Locate
#  - Registry
#  - nsDialogs
#  - winmessages
#  - MUI2
#  - Logiclib
#  - x64
# channels:
#  - nsis
# run_with: makensis
!include nsDialogs.nsh
!include MUI2.nsh
!include "winmessages.nsh"
!include LogicLib.nsh
#---------------------------------------------------------------------------------------------------
#---------------------------------------------------------------------------------------------------

; https://nsis.sourceforge.io/NsDialogs_FAQ#How_to_create_two_groups_of_RadioButtons

Var dialog
Var hwnd
Var isexpress

Function preExpressCustom
	nsDialogs::Create 1018
		Pop $dialog
 
	${NSD_CreateRadioButton} 0 0 40% 6% "Express"
		Pop $hwnd
		${NSD_AddStyle} $hwnd ${WS_GROUP}
		nsDialogs::SetUserData $hwnd "true"
		${NSD_OnClick} $hwnd RadioClick
		${NSD_Check} $hwnd
	${NSD_CreateRadioButton} 0 12% 40% 6% "Custom"
		Pop $hwnd
		nsDialogs::SetUserData $hwnd "false"
		${NSD_OnClick} $hwnd RadioClick
 
	nsDialogs::Show
FunctionEnd
 
Function RadioClick
	Pop $hwnd
	nsDialogs::GetUserData $hwnd
	Pop $hwnd
	StrCpy $isexpress $hwnd
FunctionEnd

#---------------------------------------------------------------------------------------------------

Name "HoboVR"
OutFile "hobovr_setup.exe"
XPStyle on


Var EXIT_CODE

!insertmacro MUI_PAGE_LICENSE "..\..\LICENSE"
#Page custom preExpressCustom 
Page directory
Page instfiles
UninstPage uninstConfirm
UninstPage instfiles

Function .onInit
  StrCpy $isexpress "true"
  StrCpy $INSTDIR "$PROGRAMFILES\Hobo VR"
FunctionEnd

Section
	
# Tell Steam where the VR DLLs are
IfFileExists "C:\Program Files (x86)\Steam\steamapps\common\SteamVR\bin\win32\vrpathreg.exe" PathGood
	MessageBox MB_OK|MB_ICONEXCLAMATION "Steam vr path register exe doesn't exist. has steam and steamvr been installed?"
	Quit
PathGood:
    # Create Directory
    CreateDirectory $INSTDIR
    AccessControl::GrantOnFile  "$INSTDIR" "(S-1-5-32-545)" "FullAccess"
    # Unpack the files
    SetOutPath $INSTDIR
    File /a /r "..\..\hobovr\*.*"

	ExecDos::exec /DETAILED '"C:\Program Files (x86)\Steam\steamapps\common\SteamVR\bin\win32\vrpathreg.exe" adddriver "$INSTDIR\hobovr"' $EXIT_CODE install_log.txt
	ExecDos::exec /DETAILED '"C:\Program Files (x86)\Steam\steamapps\common\SteamVR\bin\win32\vrpathreg.exe" show' $EXIT_CODE install_log.txt

# Install hobovr virtualreality python bindings
# ExecDos::exec /DETAILED '$pythonExe -m pip install -e "$INSTDIR\hobo_vr-master\bindings\python"' $EXIT_CODE install_log.txt
goto end
	
# Handle errors
hobo_failed:
	MessageBox MB_OK|MB_ICONEXCLAMATION "Unable to Install hobo_vr."
	 Quit
end:

SectionEnd
