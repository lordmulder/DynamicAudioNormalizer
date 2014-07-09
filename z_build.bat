@echo off
setlocal enabledelayedexpansion

REM ///////////////////////////////////////////////////////////////////////////
REM // Set Paths
REM ///////////////////////////////////////////////////////////////////////////
set "MSVC_PATH=C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC"
set "UPX3_PATH=C:\Program Files (x86)\UPX"
set "PDOC_PATH=C:\Program Files (x86)\Pandoc"

REM ###############################################
REM # DO NOT MODIFY ANY LINES BELOW THIS LINE !!! #
REM ###############################################

REM ///////////////////////////////////////////////////////////////////////////
REM // Setup environment
REM ///////////////////////////////////////////////////////////////////////////
call "%MSVC_PATH%\vcvarsall.bat" x86

REM ///////////////////////////////////////////////////////////////////////////
REM // Check environment
REM ///////////////////////////////////////////////////////////////////////////
if "%VCINSTALLDIR%"=="" (
	echo %%VCINSTALLDIR%% not specified. Please check your MSVC_PATH var!
	goto BuildError
)
if not exist "%VCINSTALLDIR%\bin\cl.exe" (
	echo C++ compiler binary not found. Please check your MSVC_PATH var!
	goto BuildError
)
if not exist "%UPX3_PATH%\upx.exe" (
	echo UPX binary could not be found. Please check your UPX3_PATH var!
	goto BuildError
)
if not exist "%PDOC_PATH%\pandoc.exe" (
	echo Pandoc binary could not be found. Please check your PDOC_PATH var!
	goto BuildError
)

REM ///////////////////////////////////////////////////////////////////////////
REM // Get current date and time (in ISO format)
REM ///////////////////////////////////////////////////////////////////////////
set "ISO_DATE="
set "ISO_TIME="
if not exist "%~dp0\etc\date.exe" BuildError
for /F "tokens=1,2 delims=:" %%a in ('"%~dp0\etc\date.exe" +ISODATE:%%Y-%%m-%%d') do (
	if "%%a"=="ISODATE" set "ISO_DATE=%%b"
)
for /F "tokens=1,2,3,4 delims=:" %%a in ('"%~dp0\etc\date.exe" +ISOTIME:%%T') do (
	if "%%a"=="ISOTIME" set "ISO_TIME=%%b:%%c:%%d"
)
if "%ISO_DATE%"=="" goto BuildError
if "%ISO_TIME%"=="" goto BuildError

REM ///////////////////////////////////////////////////////////////////////////
REM // Build the binaries
REM ///////////////////////////////////////////////////////////////////////////
for %%c in (DLL, Static) do (
	echo ---------------------------------------------------------------------
	echo BEGIN BUILD [Release_%%c]
	echo ---------------------------------------------------------------------

	MSBuild.exe /property:Configuration=Release_%%c /target:clean   "%~dp0\DynamicAudioNormalizer.sln"
	if not "!ERRORLEVEL!"=="0" goto BuildError
	MSBuild.exe /property:Configuration=Release_%%c /target:rebuild "%~dp0\DynamicAudioNormalizer.sln"
	if not "!ERRORLEVEL!"=="0" goto BuildError
	MSBuild.exe /property:Configuration=Release_%%c /target:build   "%~dp0\DynamicAudioNormalizer.sln"
	if not "!ERRORLEVEL!"=="0" goto BuildError
)

REM ///////////////////////////////////////////////////////////////////////////
REM // Copy program files
REM ///////////////////////////////////////////////////////////////////////////
set "PACK_PATH=%TMP%\~%RANDOM%%RANDOM%.tmp"
mkdir "%PACK_PATH%"

for %%c in (DLL, Static) do (
	echo ---------------------------------------------------------------------
	echo BEGIN PACKAGING [Release_%%c]
	echo ---------------------------------------------------------------------
	
	mkdir "%PACK_PATH%\%%c"
	mkdir "%PACK_PATH%\%%c\img"

	copy "%~dp0\bin\Win32\Release_%%c\DynamicAudioNormalizerCLI.exe" "%PACK_PATH%\%%c"

	if "%%c"=="DLL" (
		copy "%~dp0\bin\Win32\Release_%%c\DynamicAudioNormalizerAPI.dll" "%PACK_PATH%\%%c"
		copy "%~dp0\etc\sndfile\lib\Win32\shared\libsndfile-1.dll"       "%PACK_PATH%\%%c"
		copy "%MSVC_PATH%\redist\x86\Microsoft.VC120.CRT\msvc?120.dll"   "%PACK_PATH%\%%c"
	)

	copy "%~dp0\LICENSE.html" "%PACK_PATH%\%%c"
	copy "%~dp0\img\*.png"    "%PACK_PATH%\%%c\img"

	"%PDOC_PATH%\pandoc.exe" --from markdown_github+header_attributes --to html5 --standalone -H "%~dp0\img\Style.inc" "%~dp0\README.md" --output "%PACK_PATH%\%%c\README.html"
)

REM ///////////////////////////////////////////////////////////////////////////
REM // Compress
REM ///////////////////////////////////////////////////////////////////////////
for %%c in (DLL, Static) do (
	"%UPX3_PATH%\upx.exe" --best "%PACK_PATH%\%%c\*.exe"
	"%UPX3_PATH%\upx.exe" --best "%PACK_PATH%\%%c\*.dll"
)

REM ///////////////////////////////////////////////////////////////////////////
REM // Attributes
REM ///////////////////////////////////////////////////////////////////////////
for %%c in (DLL, Static) do (
	attrib +R "%PACK_PATH%\%%c\*.exe"
	attrib +R "%PACK_PATH%\%%c\*.html"
	if "%%c"=="DLL" (
		attrib +R "%PACK_PATH%\%%c\*.dll"
	)
)

REM ///////////////////////////////////////////////////////////////////////////
REM // Generate outfile name
REM ///////////////////////////////////////////////////////////////////////////
mkdir "%~dp0\out" 2> NUL
set "OUT_NAME=DynamicAudioNormalizer.%ISO_DATE%"
:CheckOutName
for %%c in (DLL, Static) do (
	if exist "%~dp0\out\%OUT_NAME%.%%c.zip" (
		set "OUT_NAME=%OUT_NAME%.new"
		goto CheckOutName
	)
)

REM ///////////////////////////////////////////////////////////////////////////
REM // Create version tag
REM ///////////////////////////////////////////////////////////////////////////
echo Dynamic Audio Normalizer > "%PACK_PATH%\BUILD_TAG"
echo Copyright (C) 2014 LoRd_MuldeR ^<MuldeR2@GMX.de^> >> "%PACK_PATH%\BUILD_TAG"
echo. >> "%PACK_PATH%\BUILD_TAG"
echo Built on %ISO_DATE%, at %TIME% >> "%PACK_PATH%\BUILD_TAG"
cl 2>&1 | "%~dp0\etc\head.exe" -n1 | "%~dp0\etc\sed.exe" "s/^/Compiler version: /" >> "%PACK_PATH%\BUILD_TAG"
echo. >> "%PACK_PATH%\BUILD_TAG"
echo This program is free software; you can redistribute it and/or modify >> "%PACK_PATH%\BUILD_TAG"
echo it under the terms of the GNU General Public License as published by >> "%PACK_PATH%\BUILD_TAG"
echo the Free Software Foundation; either version 2 of the License, or >> "%PACK_PATH%\BUILD_TAG"
echo (at your option) any later version. >> "%PACK_PATH%\BUILD_TAG"

REM ///////////////////////////////////////////////////////////////////////////
REM // Build the package
REM ///////////////////////////////////////////////////////////////////////////
for %%c in (DLL, Static) do (
	copy "%PACK_PATH%\BUILD_TAG" "%PACK_PATH%\%%c"
	pushd "%PACK_PATH%\%%c"
	"%~dp0\etc\zip.exe" -9 -r -z "%~dp0\out\%OUT_NAME%.%%c.zip" "*.*" < "%PACK_PATH%\BUILD_TAG"
	popd
	attrib +R "%~dp0\out\%OUT_NAME%.%%c.zip"
)

REM Clean up!
rmdir /Q /S "%PACK_PATH%"

REM ///////////////////////////////////////////////////////////////////////////
REM // COMPLETE
REM ///////////////////////////////////////////////////////////////////////////
echo.
echo Build completed.
echo.
pause
goto:eof

REM ///////////////////////////////////////////////////////////////////////////
REM // FAILED
REM ///////////////////////////////////////////////////////////////////////////
:BuildError
echo.
echo Build has failed !!!
echo.
pause
