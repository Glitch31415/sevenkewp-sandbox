@ECHO OFF
@rem used for GitHub Actions. Takes about 5 minutes to install.
SetLocal EnableExtensions

SET "VSINSTALLER=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vs_installer.exe"
SET "VSPATH=C:\Program Files\Microsoft Visual Studio\2022\Enterprise"

"%VSINSTALLER%" modify --installPath "%VSPATH%" --add Microsoft.VisualStudio.Component.WinXP --quiet --norestart --nocache