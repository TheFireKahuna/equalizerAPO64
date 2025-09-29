!include "LogicLib.nsh"
!include "MUI2.nsh"
!include "nsArray.nsh"
!include "NSISpcre.nsh"
!include "WinVer.nsh"
!include "x64.nsh"

;Use ANSI because NSISpcre's Unicode version is less optimized
Unicode false
ManifestDPIAware true
;Use more efficient compression
SetCompressor /SOLID lzma

!insertmacro REReplace

!searchparse /file ..\version.h `#define MAJOR ` MAJOR
!searchparse /file ..\version.h `#define MINOR ` MINOR
!searchparse /file ..\version.h `#define REVISION ` REVISION
!if ${REVISION} == 0
!define VERSION ${MAJOR}.${MINOR}
!else
!define VERSION ${MAJOR}.${MINOR}.${REVISION}
!endif

!define REGPATH "Software\EqualizerAPO"
!define UNINST_REGPATH "Software\Microsoft\Windows\CurrentVersion\Uninstall\EqualizerAPO"

;--------------------------------
;General

  ;Name and file
  Name "Equalizer APO ${VERSION}"

  ;Request application privileges for Windows Vista
  RequestExecutionLevel admin
  
;--------------------------------
;Variables

  Var StartMenuFolder
  Var OldStartMenuFolder
  Var OLDINSTDIR
  
;--------------------------------
;Interface Settings

  !define MUI_ABORTWARNING
  !define MUI_COMPONENTSPAGE_NODESC
  !define MUI_WELCOMEPAGE_TITLE_3LINES

;--------------------------------
;Pages

  !insertmacro MUI_PAGE_WELCOME
  !insertmacro MUI_PAGE_LICENSE ..\License.txt
  !insertmacro MUI_PAGE_DIRECTORY
  
;Start Menu Folder Page Configuration
  !define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKLM" 
  !define MUI_STARTMENUPAGE_REGISTRY_KEY ${REGPATH} 
  !define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"
  
  !insertmacro MUI_PAGE_STARTMENU Application $StartMenuFolder
  !insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_INSTFILES
  !insertmacro MUI_PAGE_FINISH

  !insertmacro MUI_UNPAGE_WELCOME
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_COMPONENTS
  !insertmacro MUI_UNPAGE_INSTFILES
  !insertmacro MUI_UNPAGE_FINISH

;--------------------------------
;Languages

  !insertmacro MUI_LANGUAGE "English"
  !insertmacro MUI_LANGUAGE "German"

;--------------------------------
;Macros
Var renamePath
Var renameIndex
!macro RenameAndDelete path
  ${If} ${FileExists} "${path}"
    StrCpy $renamePath "${path}.old"
    StrCpy $renameIndex "0"
    ${While} ${FileExists} "$renamePath"
      StrCpy $renamePath "${path}.old.$renameIndex"
      IntOp $renameIndex $renameIndex + 1
    ${EndWhile}
    Rename "${path}" "$renamePath"
    nsArray::Set deleteAfterRenameArray "$renamePath"
  ${EndIf}
!macroend
    
LangString VersionError ${LANG_ENGLISH} "This installer is only supposed to be run on {0} Windows. Please use the {1} installer."
LangString VersionError ${LANG_GERMAN} "Dieses Installationsprogramm kann nur auf einem {0}-Windows verwendet werden. Bitte nutzen Sie die {1}-Version."

LangString UCRTError ${LANG_ENGLISH} "Your Windows installation is missing required updates to use this program. Please install remaining Windows updates or the Visual C++ Redistributable for Visual Studio 2015 - 2022.$\n$\nDo you want to download the Visual C++ Redistributable now?"
LangString UCRTError ${LANG_GERMAN} "Ihrer Windows-Installation fehlen ben�tigte Updates, um dieses Programm zu verwenden. Bitte installieren Sie ausstehende Windows-Updates oder das Visual C++ Redistributable f�r Visual Studio 2015 - 2022.$\n$\nM�chten Sie jetzt das Visual C++ Redistributable herunterladen?"

;--------------------------------
;Functions
Function .onInit
  !if ${LIBPATH} != "lib32"
    SetRegView 64
  !endif
  ;Get installation folder from registry if available
  ReadRegStr $INSTDIR HKLM ${REGPATH} "InstallPath"

  ;Use default installation folder otherwise
  ${If} $INSTDIR == ""
    StrCpy $INSTDIR "$PROGRAMFILES64\EqualizerAPO"
  ${EndIf}
    
  !insertmacro MUI_STARTMENU_GETFOLDER Application $StartMenuFolder
  ;Try to replace version number in start menu folder
  ${REReplace} $0 "Equalizer APO [0-9]+\.[0-9]+(?:\.[0-9]+)?" "$StartMenuFolder" "Equalizer APO ${VERSION}" 1
  ${If} $0 != ""
    StrCpy $StartMenuFolder "$0"
  ${EndIf}
  
  ${If} ${IsNativeIA32}
    StrCpy $0 "x86"
  ${ElseIf} ${IsNativeAMD64}
    StrCpy $0 "x64"
  ${ElseIf} ${IsNativeARM64}
    StrCpy $0 "ARM64"
  ${EndIf}
  
  ${If} $0 != ${TARGET_ARCH}
    ${REReplace} $1 "\{0\}" $(VersionError) ${TARGET_ARCH} 1
    ${REReplace} $1 "\{1\}" $1 $0 1
    MessageBox MB_OK|MB_ICONSTOP $1
    Abort
  ${EndIf}
  
  ${IfNot} ${AtLeastWin10}
    System::Call 'KERNEL32::LoadLibrary(t "ucrtbase.dll")p.r0'
    ${If} $0 P= 0
      MessageBox MB_YESNO|MB_ICONSTOP $(UCRTError) IDNO skipDownload
      ExecShell "open" "${VCREDIST_URL}"
      skipDownload:
      Abort
    ${EndIf}
  ${EndIf}
FunctionEnd

;--------------------------------
;Installer Sections
LangString SecCheckForUpdates ${LANG_ENGLISH} "Check for updates automatically"
LangString SecCheckForUpdates ${LANG_GERMAN} "Automatisch auf Updates pr�fen"

Section $(SecCheckForUpdates) SecCheckForUpdates
SectionEnd

Section "-Install"
  SetOutPath "$INSTDIR"

  ;Possibly remove files from previous installation
  !insertmacro MUI_STARTMENU_GETFOLDER Application $OldStartMenuFolder
  RMDir /r "$SMPROGRAMS\$OldStartMenuFolder"
  
  Delete "$INSTDIR\Configurator.exe"
  Delete "$INSTDIR\Qt5Core.dll"
  Delete "$INSTDIR\Qt5Gui.dll"
  Delete "$INSTDIR\Qt5Widgets.dll"
  Delete "$INSTDIR\qt\imageformats\qgif.dll"
  Delete "$INSTDIR\qt\imageformats\qjpeg.dll"
  Delete "$INSTDIR\qt\styles\qwindowsvistastyle.dll"
  
  ;Rename before delete as these files may be in use
  !insertmacro RenameAndDelete "$INSTDIR\EqualizerAPO.dll"
  !insertmacro RenameAndDelete "$INSTDIR\libfftw3-3.dll"
  !insertmacro RenameAndDelete "$INSTDIR\fftw3.dll"
  !insertmacro RenameAndDelete "$INSTDIR\libsndfile-1.dll"
  !insertmacro RenameAndDelete "$INSTDIR\sndfile.dll"
  !insertmacro RenameAndDelete "$INSTDIR\msvcp100.dll"
  !insertmacro RenameAndDelete "$INSTDIR\msvcr100.dll"
  !insertmacro RenameAndDelete "$INSTDIR\msvcp120.dll"
  !insertmacro RenameAndDelete "$INSTDIR\msvcr120.dll"
  !insertmacro RenameAndDelete "$INSTDIR\msvcp140.dll"
  !insertmacro RenameAndDelete "$INSTDIR\msvcp140_1.dll"
  !insertmacro RenameAndDelete "$INSTDIR\msvcp140_2.dll"
  !insertmacro RenameAndDelete "$INSTDIR\VoicemeeterClient.exe"
  !insertmacro RenameAndDelete "$INSTDIR\vcruntime140.dll"
  !insertmacro RenameAndDelete "$INSTDIR\vcruntime140_1.dll"
  
  File "${BINPATH}\EqualizerAPO.dll"
  File "${BINPATH}\DeviceSelector.exe"
  File "${BINPATH}\Benchmark.exe"
  File "${BINPATH}\VoicemeeterClient.exe"
  File "${BINPATH}\UpdateChecker.exe"
  
  File "${BINPATH_EDITOR}\Editor.exe"
  
  File "${LIBPATH}\fftw3.dll"
  File "${LIBPATH}\sndfile.dll"
  File "${LIBPATH}\msvcp140.dll"
  File "${LIBPATH}\msvcp140_1.dll"
  File "${LIBPATH}\msvcp140_2.dll"
  File "${LIBPATH}\Qt6Core.dll"
  File "${LIBPATH}\Qt6Gui.dll"
  File "${LIBPATH}\Qt6Network.dll"
  File "${LIBPATH}\Qt6Svg.dll"
  File "${LIBPATH}\Qt6Widgets.dll"
  File "${LIBPATH}\vcruntime140.dll"
  !if ${LIBPATH} != "lib32"
	File "${LIBPATH}\vcruntime140_1.dll"
  !endif
  
  CreateDirectory "$INSTDIR\qt"
  CreateDirectory "$INSTDIR\qt\iconengines"
  CreateDirectory "$INSTDIR\qt\imageformats"
  CreateDirectory "$INSTDIR\qt\platforms"
  CreateDirectory "$INSTDIR\qt\styles"
  CreateDirectory "$INSTDIR\qt\tls"
  
  File /oname=qt\iconengines\qsvgicon.dll "${LIBPATH}\qt\iconengines\qsvgicon.dll"
  File /oname=qt\imageformats\qico.dll "${LIBPATH}\qt\imageformats\qico.dll"
  File /oname=qt\imageformats\qsvg.dll "${LIBPATH}\qt\imageformats\qsvg.dll"
  File /oname=qt\platforms\qwindows.dll "${LIBPATH}\qt\platforms\qwindows.dll"
  File /oname=qt\styles\qmodernwindowsstyle.dll "${LIBPATH}\qt\styles\qmodernwindowsstyle.dll"
  File /oname=qt\tls\qschannelbackend.dll "${LIBPATH}\qt\tls\qschannelbackend.dll"
  
  File "Configuration tutorial (online).url"
  File "Configuration reference (online).url"
  
  CreateDirectory "$INSTDIR\config"
  CreateDirectory "$INSTDIR\VSTPlugins"
  
  SetOverwrite off
  File /oname=config\config.txt "config\config.txt"
  File /oname=config\example.txt "config\example.txt"
  File /oname=config\demo.txt "config\demo.txt"
  File /oname=config\multichannel.txt "config\multichannel.txt"
  File /oname=config\iir_lowpass.txt "config\iir_lowpass.txt"
  File /oname=config\selective_delay.txt "config\selective_delay.txt"
  SetOverwrite on

  ;Grant write access to the config directory for all users
  AccessControl::GrantOnFile "$INSTDIR\config" "(S-1-5-32-545)" "FullAccess"

  ReadRegStr $OLDINSTDIR HKLM ${REGPATH} "InstallPath"
  WriteRegStr HKLM ${REGPATH} "InstallPath" "$INSTDIR"
  
  ;Write ConfigPath if non-existing or if InstallPath has changed
  ReadRegStr $0 HKLM ${REGPATH} "ConfigPath"
  ${If} $0 == ""
  ${OrIf} $INSTDIR != $OLDINSTDIR
	WriteRegStr HKLM ${REGPATH} "ConfigPath" "$INSTDIR\config"
  ${EndIf}
	
  ReadRegStr $0 HKLM ${REGPATH} "EnableTrace"
  ${If} $0 == ""
	WriteRegStr HKLM ${REGPATH} "EnableTrace" "false"
  ${EndIf}

  WriteUninstaller "$INSTDIR\Uninstall.exe"
  
  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
  ;Create shortcuts
  CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
  CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Equalizer APO Configuration Editor.lnk" "$INSTDIR\Editor.exe"
  CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Configuration tutorial (online).lnk" "$INSTDIR\Configuration tutorial (online).url"
  CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Configuration reference (online).lnk" "$INSTDIR\Configuration reference (online).url"
  CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Equalizer APO Device Selector.lnk" "$INSTDIR\DeviceSelector.exe"
  CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Benchmark.lnk" "$INSTDIR\Benchmark.exe"
  CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
  !insertmacro MUI_STARTMENU_WRITE_END
  
  WriteRegStr HKLM ${UNINST_REGPATH} "DisplayName" "Equalizer APO"
  WriteRegStr HKLM ${UNINST_REGPATH} "DisplayVersion" "${VERSION}"
  WriteRegStr HKLM ${UNINST_REGPATH} "UninstallString" '"$INSTDIR\Uninstall.exe"'
  WriteRegDWORD HKLM ${UNINST_REGPATH} "NoModify" 1
  WriteRegDWORD HKLM ${UNINST_REGPATH} "NoRepair" 1

  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Audio" "DisableProtectedAudioDG" 1
  ;RegDLL doesn't work for 64 bit dlls
  ExecWait '"$SYSDIR\regsvr32.exe" /s "$INSTDIR\EqualizerAPO.dll"'

  ExecWait '"$INSTDIR\DeviceSelector.exe" /i' $0
  
  ${If} ${SectionIsSelected} ${SecCheckForUpdates}
    ExecWait '"$INSTDIR\UpdateChecker.exe" -i'
  ${Else}
    ExecWait '"$INSTDIR\UpdateChecker.exe" -u'
  ${EndIf}

  ;Hopefully, the renamed files can be deleted without reboot after the Device Selector has restarted the audio service
  ${ForEachIn} deleteAfterRenameArray $R0 $R1
    Delete /REBOOTOK "$R1"
  ${Next}
  
  ${If} $0 == "0"
    SetRebootFlag false
  ${Else}
    SetRebootFlag true
  ${EndIf}
  
SectionEnd

;--------------------------------
;Uninstaller Sections

LangString SecRemoveName ${LANG_ENGLISH} "Remove configurations and registry backups"
LangString SecRemoveName ${LANG_GERMAN} "Konfigurationen und Registrierungsbackups entfernen"

Section /o un.$(SecRemoveName)
  
  Delete "$INSTDIR\*.reg"
  RMDir /REBOOTOK /r "$INSTDIR\config"
  DeleteRegKey HKCU ${REGPATH}
  
SectionEnd

Section "-un.Uninstall"
  !if ${LIBPATH} != "lib32"
	SetRegView 64
  !endif

  ;Qt applications only work if working directory is set to application directory
  Push $OUTDIR
  SetOutPath $INSTDIR
  ExecWait '"$INSTDIR\UpdateChecker.exe" -u'
  ExecWait '"$INSTDIR\DeviceSelector.exe" /u'
  Pop $OUTDIR
  SetOutPath $OUTDIR
  
  ExecWait '"$SYSDIR\regsvr32.exe" /u /s "$INSTDIR\EqualizerAPO.dll"'
  DeleteRegValue HKLM "Software\Microsoft\Windows\CurrentVersion\Audio" "DisableProtectedAudioDG"
  
  !insertmacro MUI_STARTMENU_GETFOLDER Application $StartMenuFolder
  RMDir /r "$SMPROGRAMS\$StartMenuFolder"
  
  RMDir "$INSTDIR\VSTPlugins"
  
  Delete "$INSTDIR\Configuration reference (online).url"
  Delete "$INSTDIR\Configuration tutorial (online).url"
  
  RMDir /r "$INSTDIR\qt"
  
  !if ${LIBPATH} != "lib32"
    Delete /REBOOTOK "$INSTDIR\vcruntime140_1.dll"
  !endif
  Delete /REBOOTOK "$INSTDIR\vcruntime140.dll"
  Delete "$INSTDIR\Qt6Widgets.dll"
  Delete "$INSTDIR\Qt6Svg.dll"
  Delete "$INSTDIR\Qt6Network.dll"
  Delete "$INSTDIR\Qt6Gui.dll"
  Delete "$INSTDIR\Qt6Core.dll"
  Delete /REBOOTOK "$INSTDIR\msvcp140_2.dll"
  Delete /REBOOTOK "$INSTDIR\msvcp140_1.dll"
  Delete /REBOOTOK "$INSTDIR\msvcp140.dll"
  Delete /REBOOTOK "$INSTDIR\sndfile.dll"
  Delete /REBOOTOK "$INSTDIR\fftw3.dll"
  Delete "$INSTDIR\Editor.exe"
  
  Delete "$INSTDIR\UpdateChecker.exe"
  Delete "$INSTDIR\VoicemeeterClient.exe"
  Delete "$INSTDIR\Benchmark.exe"
  Delete "$INSTDIR\DeviceSelector.exe"
  Delete /REBOOTOK "$INSTDIR\EqualizerAPO.dll"

  Delete "$INSTDIR\Uninstall.exe"

  ;Only remove if empty
  RMDir /REBOOTOK "$INSTDIR"

  DeleteRegKey HKLM ${UNINST_REGPATH}
  DeleteRegKey /ifempty HKLM ${REGPATH}

SectionEnd
