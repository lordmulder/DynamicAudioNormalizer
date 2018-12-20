@echo off
setlocal enabledelayedexpansion

REM ///////////////////////////////////////////////////////////////////////////
REM // Set Paths
REM ///////////////////////////////////////////////////////////////////////////
set "MSVC_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC"
set "TOOLS_VER=141"
set "SLN_SUFFX=VS2017"

REM ###############################################
REM # DO NOT MODIFY ANY LINES BELOW THIS LINE !!! #
REM ###############################################

REM ///////////////////////////////////////////////////////////////////////////
REM // Check environment
REM ///////////////////////////////////////////////////////////////////////////
if not exist "%MSVC_PATH%\vcvarsall.bat" (
	if not exist "%MSVC_PATH%\Auxiliary\Build\vcvarsall.bat" (
		echo MSVC compiler not found. Please check your MSVC_PATH var^^!
		goto BuildError
	)
)

if not exist "%QTDIR%\bin\moc.exe" (
	echo Qt could not be found. Please check your QTDIR var^^!
	goto BuildError
)
if not exist "%QTDIR%\include\QtCore\qglobal.h" (
	echo Qt could not be found. Please check your QTDIR var^^!
	goto BuildError
)

if not exist "%JAVA_HOME%\bin\java.exe" (
	echo JDK could not be found. Please check your JAVA_HOME var^^!
	goto BuildError
)
if not exist "%JAVA_HOME%\lib\tools.jar" (
	echo JDK could not be found. Please check your JAVA_HOME var^^!
	goto BuildError
)
if not exist "%JAVA_HOME%\include\jni.h" (
	echo JDK could not be found. Please check your JAVA_HOME var^^!
	goto BuildError
)

if not exist "%~dp0\..\Prerequisites\README.txt" (
	echo Prerequisites not found. Repair your build environment^^!
	goto BuildError
)

REM ///////////////////////////////////////////////////////////////////////////
REM // Setup environment
REM ///////////////////////////////////////////////////////////////////////////
set "ANT_HOME=%~dp0\..\Prerequisites\Ant"
set "PATH=%QTDIR%\bin;%JAVA_HOME%\bin;%ANT_HOME%\bin;%PATH%"

if exist "%MSVC_PATH%\vcvarsall.bat" (
	call "%MSVC_PATH%\vcvarsall.bat" x86
) else (
	if exist "%MSVC_PATH%\Auxiliary\Build\vcvarsall.bat" (
		call "%MSVC_PATH%\Auxiliary\Build\vcvarsall.bat" x86
	)
)

if not exist "%VCINSTALLDIR%\bin\cl.exe" (
	if not exist "%VCToolsInstallDir%\bin\Hostx64\x86\cl.exe" (
		echo MSVC compiler not found. Please check your MSVC_PATH var^^!
		goto BuildError
	)
)

REM ///////////////////////////////////////////////////////////////////////////
REM // Get current date and time (in ISO format)
REM ///////////////////////////////////////////////////////////////////////////
set "ISO_DATE="
set "ISO_TIME="
if not exist "%~dp0\..\Prerequisites\GnuWin32\date.exe" goto BuildError
for /F "tokens=1,2 delims=:" %%a in ('"%~dp0\..\Prerequisites\GnuWin32\date.exe" +ISODATE:%%Y-%%m-%%d') do (
	if "%%a"=="ISODATE" set "ISO_DATE=%%b"
)
for /F "tokens=1,2,3,4 delims=:" %%a in ('"%~dp0\..\Prerequisites\GnuWin32\date.exe" +ISOTIME:%%T') do (
	if "%%a"=="ISOTIME" set "ISO_TIME=%%b:%%c:%%d"
)
if "%ISO_DATE%"=="" goto BuildError
if "%ISO_TIME%"=="" goto BuildError

REM ///////////////////////////////////////////////////////////////////////////
REM // Detect version number
REM ///////////////////////////////////////////////////////////////////////////
set "VER_MAJOR="
set "VER_MINOR="
set "VER_PATCH="
for /F "tokens=4,5,6 delims=; " %%a in (%~dp0\DynamicAudioNormalizerAPI\src\Version.cpp) do (
	if "%%b"=="=" (
		if "%%a"=="DYNAUDNORM_NS::VERSION_MAJOR" set "VER_MAJOR=%%c"
		if "%%a"=="DYNAUDNORM_NS::VERSION_MINOR" set "VER_MINOR=%%c"
		if "%%a"=="DYNAUDNORM_NS::VERSION_PATCH" set "VER_PATCH=%%c"
	)
)
if "%VER_MAJOR%"=="" goto BuildError
if "%VER_MINOR%"=="" goto BuildError
if "%VER_PATCH%"=="" goto BuildError
if %VER_MINOR% LSS 10 set "VER_MINOR=0%VER_MINOR%

REM ///////////////////////////////////////////////////////////////////////////
REM // Clean Binaries
REM ///////////////////////////////////////////////////////////////////////////
for /d %%d in (%~dp0\bin\*) do (rmdir /S /Q "%%~d")
for /d %%d in (%~dp0\obj\*) do (rmdir /S /Q "%%~d")

REM ///////////////////////////////////////////////////////////////////////////
REM // Build the binaries
REM ///////////////////////////////////////////////////////////////////////////
for %%c in (DLL, Static) do (
	for %%p in (Win32, x64) do (
		echo ---------------------------------------------------------------------
		echo BEGIN BUILD [%%p/Release_%%c]
		echo ---------------------------------------------------------------------

		MSBuild.exe /property:Platform=%%p /property:Configuration=Release_%%c /target:clean   "%~dp0\DynamicAudioNormalizer_%SLN_SUFFX%.sln"
		if not "!ERRORLEVEL!"=="0" goto BuildError
		MSBuild.exe /property:Platform=%%p /property:Configuration=Release_%%c /target:rebuild "%~dp0\DynamicAudioNormalizer_%SLN_SUFFX%.sln"
		if not "!ERRORLEVEL!"=="0" goto BuildError
		MSBuild.exe /property:Platform=%%p /property:Configuration=Release_%%c /target:build   "%~dp0\DynamicAudioNormalizer_%SLN_SUFFX%.sln"
		if not "!ERRORLEVEL!"=="0" goto BuildError
	)
)

call "%ANT_HOME%\bin\ant.bat" -buildfile "%~dp0\DynamicAudioNormalizerJNI\build.xml"
if not "!ERRORLEVEL!"=="0" goto BuildError

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
	mkdir "%PACK_PATH%\%%c\x64"
	mkdir "%PACK_PATH%\%%c\img"
	mkdir "%PACK_PATH%\%%c\img\dyauno"
	mkdir "%PACK_PATH%\%%c\include"
	mkdir "%PACK_PATH%\%%c\samples"
	mkdir "%PACK_PATH%\%%c\samples\python"

	copy "%~dp0\bin\Win32\v%TOOLS_VER%_xp\Release_%%c\DynamicAudioNormalizerCLI.exe"            "%PACK_PATH%\%%c"
	if not "!ERRORLEVEL!"=="0" goto BuildError
	copy "%~dp0\bin\Win32\v%TOOLS_VER%_xp\Release_%%c\DynamicAudioNormalizerGUI.exe"            "%PACK_PATH%\%%c"
	if not "!ERRORLEVEL!"=="0" goto BuildError
	copy "%~dp0\bin\Win32\v%TOOLS_VER%_xp\Release_%%c\DynamicAudioNormalizerPYD.pyd"            "%PACK_PATH%\%%c"
	if not "!ERRORLEVEL!"=="0" goto BuildError
	copy "%~dp0\bin\x64\.\v%TOOLS_VER%_xp\Release_%%c\DynamicAudioNormalizerPYD.pyd"            "%PACK_PATH%\%%c\x64"
	if not "!ERRORLEVEL!"=="0" goto BuildError
	copy "%~dp0\bin\Win32\v%TOOLS_VER%_xp\Release_%%c\DynamicAudioNormalizerVST.dll"            "%PACK_PATH%\%%c"
	if not "!ERRORLEVEL!"=="0" goto BuildError
	copy "%~dp0\bin\x64\.\v%TOOLS_VER%_xp\Release_%%c\DynamicAudioNormalizerVST.dll"            "%PACK_PATH%\%%c\x64"
	if not "!ERRORLEVEL!"=="0" goto BuildError
	copy "%~dp0\bin\Win32\v%TOOLS_VER%_xp\Release_%%c\DynamicAudioNormalizerWA5.dll"            "%PACK_PATH%\%%c"
	if not "!ERRORLEVEL!"=="0" goto BuildError
	
	copy "%~dp0\DynamicAudioNormalizerPYD\include\*.py"                                         "%PACK_PATH%\%%c\include"
	if not "!ERRORLEVEL!"=="0" goto BuildError
	copy "%~dp0\DynamicAudioNormalizerPYD\samples\*.py"                                         "%PACK_PATH%\%%c\samples\python"
	if not "!ERRORLEVEL!"=="0" goto BuildError

	if /I "%%c"=="DLL" (
		mkdir "%PACK_PATH%\%%c\redist"
		mkdir "%PACK_PATH%\%%c\samples\delphi"
		mkdir "%PACK_PATH%\%%c\samples\dotNet"
		mkdir "%PACK_PATH%\%%c\samples\java"
		
		copy "%~dp0\bin\Win32\v%TOOLS_VER%_xp\Release_%%c\DynamicAudioNormalizerAPI.dll"        "%PACK_PATH%\%%c"
		if not "!ERRORLEVEL!"=="0" goto BuildError
		copy "%~dp0\bin\Win32\v%TOOLS_VER%_xp\Release_%%c\DynamicAudioNormalizerAPI.lib"        "%PACK_PATH%\%%c"
		if not "!ERRORLEVEL!"=="0" goto BuildError
		copy "%~dp0\bin\Win32\v%TOOLS_VER%_xp\Release_%%c\DynamicAudioNormalizerNET.dll"        "%PACK_PATH%\%%c"
		if not "!ERRORLEVEL!"=="0" goto BuildError
		copy "%~dp0\bin\x64\.\v%TOOLS_VER%_xp\Release_%%c\DynamicAudioNormalizerAPI.dll"        "%PACK_PATH%\%%c\x64"
		if not "!ERRORLEVEL!"=="0" goto BuildError
		copy "%~dp0\bin\x64\.\v%TOOLS_VER%_xp\Release_%%c\DynamicAudioNormalizerAPI.lib"        "%PACK_PATH%\%%c\x64"
		if not "!ERRORLEVEL!"=="0" goto BuildError
		copy "%~dp0\bin\x64\.\v%TOOLS_VER%_xp\Release_%%c\DynamicAudioNormalizerNET.dll"        "%PACK_PATH%\%%c\x64"
		if not "!ERRORLEVEL!"=="0" goto BuildError
		copy "%~dp0\DynamicAudioNormalizerAPI\include\*.h"                                      "%PACK_PATH%\%%c\include"
		if not "!ERRORLEVEL!"=="0" goto BuildError
		copy "%~dp0\DynamicAudioNormalizerPAS\include\*.pas"                                    "%PACK_PATH%\%%c\include"
		if not "!ERRORLEVEL!"=="0" goto BuildError
		copy "%~dp0\DynamicAudioNormalizerJNI\out\*.jar"                                        "%PACK_PATH%\%%c"
		if not "!ERRORLEVEL!"=="0" goto BuildError
		copy "%~dp0\..\Prerequisites\LibSndFile\bin\Win32\libsndfile-1.dll"                     "%PACK_PATH%\%%c"
		if not "!ERRORLEVEL!"=="0" goto BuildError
		copy "%~dp0\..\Prerequisites\LibMpg123\bin\Win32\libmpg123.v%TOOLS_VER%_xp.dll"         "%PACK_PATH%\%%c\libmpg123.dll"
		if not "!ERRORLEVEL!"=="0" goto BuildError
		copy "%~dp0\..\Prerequisites\Qt4\v%TOOLS_VER%_xp\Shared\bin\QtGui4.dll"                 "%PACK_PATH%\%%c"
		if not "!ERRORLEVEL!"=="0" goto BuildError
		copy "%~dp0\..\Prerequisites\Qt4\v%TOOLS_VER%_xp\Shared\bin\QtCore4.dll"                "%PACK_PATH%\%%c"
		if not "!ERRORLEVEL!"=="0" goto BuildError
		copy "%~dp0\DynamicAudioNormalizerNET\samples\*.cs"                                     "%PACK_PATH%\%%c\samples\dotNet"
		if not "!ERRORLEVEL!"=="0" goto BuildError
		copy "%~dp0\DynamicAudioNormalizerNET\samples\*.vb"                                     "%PACK_PATH%\%%c\samples\dotNet"
		if not "!ERRORLEVEL!"=="0" goto BuildError
		copy "%~dp0\DynamicAudioNormalizerPAS\src\*.pas"                                        "%PACK_PATH%\%%c\samples\delphi"
		if not "!ERRORLEVEL!"=="0" goto BuildError
		copy "%~dp0\DynamicAudioNormalizerPAS\src\*.dfm"                                        "%PACK_PATH%\%%c\samples\delphi"
		if not "!ERRORLEVEL!"=="0" goto BuildError
		copy "%~dp0\DynamicAudioNormalizerJNI\samples\com\muldersoft\dynaudnorm\samples\*.java" "%PACK_PATH%\%%c\samples\java"
		if not "!ERRORLEVEL!"=="0" goto BuildError
		
		if %TOOLS_VER% GEQ 140 (
			if %TOOLS_VER% GEQ 141 (
				copy "%VCToolsRedistDir%\x86\Microsoft.VC%TOOLS_VER%.CRT\*.dll"                 "%PACK_PATH%\%%c"
				if not "!ERRORLEVEL!"=="0" goto BuildError
				copy "%VCToolsRedistDir%\x64\Microsoft.VC%TOOLS_VER%.CRT\*.dll"                 "%PACK_PATH%\%%c\x64"
				if not "!ERRORLEVEL!"=="0" goto BuildError
				copy "%VCToolsRedistDir%\vcredist_*.exe"                                        "%PACK_PATH%\%%c\redist"
				if not "!ERRORLEVEL!"=="0" goto BuildError
			) else (
				copy "%MSVC_PATH%\redist\x86\Microsoft.VC%TOOLS_VER%.CRT\*.dll"                 "%PACK_PATH%\%%c"
				if not "!ERRORLEVEL!"=="0" goto BuildError
				copy "%MSVC_PATH%\redist\x64\Microsoft.VC%TOOLS_VER%.CRT\*.dll"                 "%PACK_PATH%\%%c\x64"
				if not "!ERRORLEVEL!"=="0" goto BuildError
				copy "%MSVC_PATH%\redist\1033\vcredist_*.exe"                                   "%PACK_PATH%\%%c\redist"
				if not "!ERRORLEVEL!"=="0" goto BuildError
			)
			copy "%~dp0\..\Prerequisites\MSVC\redist\ucrt\DLLs\x86\*.dll"                       "%PACK_PATH%\%%c"
			if not "!ERRORLEVEL!"=="0" goto BuildError
			copy "%~dp0\..\Prerequisites\MSVC\redist\ucrt\DLLs\x64\*.dll"                       "%PACK_PATH%\%%c\x64"
			if not "!ERRORLEVEL!"=="0" goto BuildError
		)
	)

	copy "%~dp0\LICENSE-LGPL.html"                                                              "%PACK_PATH%\%%c"
	if not "!ERRORLEVEL!"=="0" goto BuildError
	copy "%~dp0\LICENSE-GPL2.html"                                                              "%PACK_PATH%\%%c"
	if not "!ERRORLEVEL!"=="0" goto BuildError
	copy "%~dp0\LICENSE-GPL3.html"                                                              "%PACK_PATH%\%%c"
	if not "!ERRORLEVEL!"=="0" goto BuildError
	copy "%~dp0\img\dyauno\*.png"                                                               "%PACK_PATH%\%%c\img\dyauno"
	if not "!ERRORLEVEL!"=="0" goto BuildError

	"%~dp0\..\Prerequisites\Pandoc\pandoc.exe" --from markdown_github+pandoc_title_block+header_attributes+implicit_figures --to html5 --toc -N --standalone -H "%~dp0\..\Prerequisites\Pandoc\css\github-pandoc.inc" "%~dp0\README.md" | "%JAVA_HOME%\bin\java.exe" -jar "%~dp0\..\Prerequisites\HTMLCompressor\bin\htmlcompressor-1.5.3.jar" --compress-css -o "%PACK_PATH%\%%c\README.html"
	if not "!ERRORLEVEL!"=="0" goto BuildError
)

REM ///////////////////////////////////////////////////////////////////////////
REM // Compress
REM ///////////////////////////////////////////////////////////////////////////
for %%c in (DLL, Static) do (
	for %%i in (DynamicAudioNormalizerCLI.exe,DynamicAudioNormalizerGUI.exe,DynamicAudioNormalizerVST.dll,DynamicAudioNormalizerWA5.dll) do (
		"%~dp0\..\Prerequisites\UPX\upx.exe" --best "%PACK_PATH%\%%c\%%~i"
		if not "!ERRORLEVEL!"=="0" goto BuildError
	)
	for %%i in (DynamicAudioNormalizerVST.dll) do (
		"%~dp0\..\Prerequisites\UPX\upx.exe" --best "%PACK_PATH%\%%c\x64\%%~i"
		if not "!ERRORLEVEL!"=="0" goto BuildError
	)

	if /I "%%c"=="DLL" (
		for %%i in (DynamicAudioNormalizerAPI.dll,libmpg123.dll,libsndfile-1.dll,QtCore4.dll,QtGui4.dll) do (
			"%~dp0\..\Prerequisites\UPX\upx.exe" --best "%PACK_PATH%\%%c\%%~i"
			if not "!ERRORLEVEL!"=="0" goto BuildError
		)
		for %%i in (DynamicAudioNormalizerAPI.dll) do (
			"%~dp0\..\Prerequisites\UPX\upx.exe" --best "%PACK_PATH%\%%c\x64\%%~i"
			if not "!ERRORLEVEL!"=="0" goto BuildError
		)
	)
)

REM ///////////////////////////////////////////////////////////////////////////
REM // Create version tag
REM ///////////////////////////////////////////////////////////////////////////
echo Dynamic Audio Normalizer>                                                                                                                     "%PACK_PATH%\BUILD_TAG"
echo Copyright (c) 2014-2018 LoRd_MuldeR ^<MuldeR2@GMX.de^>. Some rights reserved.>>                                                               "%PACK_PATH%\BUILD_TAG"
echo.>>                                                                                                                                            "%PACK_PATH%\BUILD_TAG"
echo Version %VER_MAJOR%.%VER_MINOR%-%VER_PATCH%. Built on %ISO_DATE%, at %ISO_TIME%>>                                                             "%PACK_PATH%\BUILD_TAG"
echo.>>                                                                                                                                            "%PACK_PATH%\BUILD_TAG"
cl.exe 2>&1 | "%~dp0\..\Prerequisites\GnuWin32\head.exe" -n1 | "%~dp0\..\Prerequisites\GnuWin32\sed.exe" -e "/^$/d" -e "s/^/Compiler version: /">> "%PACK_PATH%\BUILD_TAG"
ver    2>&1 |                                                  "%~dp0\..\Prerequisites\GnuWin32\sed.exe" -e "/^$/d" -e "s/^/Build platform:   /">> "%PACK_PATH%\BUILD_TAG"
echo.>>                                                                                                                                            "%PACK_PATH%\BUILD_TAG"
echo This library is free software; you can redistribute it and/or>>                                                                               "%PACK_PATH%\BUILD_TAG"
echo modify it under the terms of the GNU Lesser General Public>>                                                                                  "%PACK_PATH%\BUILD_TAG"
echo License as published by the Free Software Foundation; either>>                                                                                "%PACK_PATH%\BUILD_TAG"
echo version 2.1 of the License, or (at your option) any later version.>>                                                                          "%PACK_PATH%\BUILD_TAG"
echo.>>                                                                                                                                            "%PACK_PATH%\BUILD_TAG"
echo This library is distributed in the hope that it will be useful,>>                                                                             "%PACK_PATH%\BUILD_TAG"
echo but WITHOUT ANY WARRANTY; without even the implied warranty of>>                                                                              "%PACK_PATH%\BUILD_TAG"
echo MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU>>                                                                           "%PACK_PATH%\BUILD_TAG"
echo Lesser General Public License for more details.>>                                                                                             "%PACK_PATH%\BUILD_TAG"

REM ///////////////////////////////////////////////////////////////////////////
REM // Attributes
REM ///////////////////////////////////////////////////////////////////////////
for %%c in (DLL, Static) do (
	attrib +R "%PACK_PATH%\%%c\*"
	attrib +R "%PACK_PATH%\%%c\x64\*"
	attrib +R "%PACK_PATH%\%%c\img\dyauno\*"
	if /I "%%c"=="DLL" (
		attrib +R "%PACK_PATH%\%%c\include\*"
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
REM // Build the package
REM ///////////////////////////////////////////////////////////////////////////
for %%c in (DLL, Static) do (
	copy "%PACK_PATH%\BUILD_TAG" "%PACK_PATH%\%%c"
	pushd "%PACK_PATH%\%%c"
	"%~dp0\..\Prerequisites\GnuWin32\zip.exe" -9 -r -z "%~dp0\out\%OUT_NAME%.%%c.zip" "*.*" < "%PACK_PATH%\BUILD_TAG"
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
echo Build has failed ^^!^^!^^!
echo.
pause
