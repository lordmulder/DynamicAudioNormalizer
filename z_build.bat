@echo off
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
	echo C++ compiler not found. Please check your MSVC_PATH var!
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
echo ---------------------------------------------------------------------
echo BEGIN BUILD
echo ---------------------------------------------------------------------
MSBuild.exe /property:Configuration=Release /target:clean   "%~dp0\DynamicAudioNormalizer.sln"
if not "%ERRORLEVEL%"=="0" goto BuildError
MSBuild.exe /property:Configuration=Release /target:rebuild "%~dp0\DynamicAudioNormalizer.sln"
if not "%ERRORLEVEL%"=="0" goto BuildError
MSBuild.exe /property:Configuration=Release /target:build   "%~dp0\DynamicAudioNormalizer.sln"
if not "%ERRORLEVEL%"=="0" goto BuildError

REM ///////////////////////////////////////////////////////////////////////////
REM // Copy base files
REM ///////////////////////////////////////////////////////////////////////////
echo ---------------------------------------------------------------------
echo BEGIN PACKAGING
echo ---------------------------------------------------------------------
set "PACK_PATH=%TMP%\~%RANDOM%%RANDOM%.tmp"
mkdir "%PACK_PATH%"
mkdir "%PACK_PATH%\img"

REM Copy program files
copy "%~dp0\bin\Win32\Release\DynamicAudioNormalizerCLI.exe" "%PACK_PATH%"
copy "%~dp0\bin\Win32\Release\DynamicAudioNormalizerAPI.dll" "%PACK_PATH%"

REM Copy dependencies
copy "%~dp0\etc\sndfile\lib\Win32\libsndfile-1.dll"            "%PACK_PATH%"
copy "%MSVC_PATH%\redist\x86\Microsoft.VC120.CRT\msvc?120.dll" "%PACK_PATH%"

REM Generate docs
copy "%~dp0\LICENSE.html" "%PACK_PATH%"
copy "%~dp0\img\*.png" "%PACK_PATH%\img"
copy "%~dp0\img\*.css" "%PACK_PATH%\img"
"%PDOC_PATH%\pandoc.exe" --from markdown_github --to html --standalone "%~dp0\README.md" --output "%PACK_PATH%\README.html"

REM ///////////////////////////////////////////////////////////////////////////
REM // Compress
REM ///////////////////////////////////////////////////////////////////////////
"%UPX3_PATH%\upx.exe" --best "%PACK_PATH%\DynamicAudioNormalizerCLI.exe"
"%UPX3_PATH%\upx.exe" --best "%PACK_PATH%\DynamicAudioNormalizerAPI.dll"
"%UPX3_PATH%\upx.exe" --best "%PACK_PATH%\libsndfile-1.dll"

REM ///////////////////////////////////////////////////////////////////////////
REM // Attributes
REM ///////////////////////////////////////////////////////////////////////////
attrib +R "%PACK_PATH%\*.exe"
attrib +R "%PACK_PATH%\*.html"
attrib +R "%PACK_PATH%\*.dll"

REM ///////////////////////////////////////////////////////////////////////////
REM // Generate outfile name
REM ///////////////////////////////////////////////////////////////////////////
mkdir "%~dp0\out" 2> NUL
set "OUT_NAME=DynamicAudioNormalizer.%ISO_DATE%"
:CheckOutName
if exist "%~dp0\out\%OUT_NAME%.zip" (
	set "OUT_NAME=%OUT_NAME%.new"
	goto CheckOutName
)

REM ///////////////////////////////////////////////////////////////////////////
REM // Create version tag
REM ///////////////////////////////////////////////////////////////////////////
echo Dynamic Audio Normalizer > "%~dp0\out\%OUT_NAME%.txt"
echo Copyright (C) 2014 LoRd_MuldeR ^<MuldeR2@GMX.de^> >> "%~dp0\out\%OUT_NAME%.txt"
echo Built %ISO_DATE%, %TIME% >> "%~dp0\out\%OUT_NAME%.txt"
echo. >> "%~dp0\out\%OUT_NAME%.txt"
echo This program is free software; you can redistribute it and/or modify >> "%~dp0\out\%OUT_NAME%.txt"
echo it under the terms of the GNU General Public License as published by >> "%~dp0\out\%OUT_NAME%.txt"
echo the Free Software Foundation; either version 2 of the License, or >> "%~dp0\out\%OUT_NAME%.txt"
echo (at your option) any later version. >> "%~dp0\out\%OUT_NAME%.txt"

REM ///////////////////////////////////////////////////////////////////////////
REM // Build the package
REM ///////////////////////////////////////////////////////////////////////////
pushd "%PACK_PATH%
"%~dp0\etc\zip.exe" -9 -r -z "%~dp0\out\%OUT_NAME%.zip" "*.*" < "%~dp0\out\%OUT_NAME%.txt"
popd
attrib +R "%~dp0\out\%OUT_NAME%.zip"
del "%~dp0\out\%OUT_NAME%.txt"

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
