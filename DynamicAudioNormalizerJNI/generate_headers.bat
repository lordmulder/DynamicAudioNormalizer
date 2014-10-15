@echo off

if not exist "%JDK_PATH%\bin\javah.exe" (
	echo Could not find 'javah.exe', please check your JDK_PATH environment variable!
	pause & exit
)

mkdir "%~dp0\include" 2> NUL
"%JDK_PATH%\bin\javah.exe" -d "%~dp0\include" -cp "%~dp0\bin" com.muldersoft.dynaudnorm.JDynamicAudioNormalizer

pause
