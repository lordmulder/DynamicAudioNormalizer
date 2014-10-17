@echo off

if not exist "%JDK8_PATH%\bin\javah.exe" (
	echo Could not find 'javah.exe', please check your JDK_PATH environment variable!
	pause & exit
)

echo Generating JNI headers, please stand by...
echo.

mkdir "%~dp0\include" 2> NUL
del /F /Q "%~dp0\include\*.h" 2> NUL

"%JDK8_PATH%\bin\javah.exe" -d "%~dp0\include" -cp "%~dp0\bin" com.muldersoft.dynaudnorm.JDynamicAudioNormalizer

if not "%ERRORLEVEL%"=="0" (
	echo.
	echo Whoops: Something went wrong !!!
	echo.
	goto TheEndMyFriend
)

echo Completed.
echo.

:TheEndMyFriend
pause
