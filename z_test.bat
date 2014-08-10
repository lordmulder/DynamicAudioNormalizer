@echo off

rmdir /Q /S "%~dp0\tmp" 2> NUL
mkdir "%~dp0\tmp"

call:runTest default
call:runTest dc-corr --correct-dc
call:runTest alt-bnd --alt-boundary
call:runTest dcc+alt --correct-dc --alt-boundary
call:runTest rms-val --target-rms 0.2
call:runTest dcc+rms --correct-dc --target-rms 0.2

goto:done

:: ============================================================================

:runTest

echo ###########################################################################
echo Test: %1
echo Args: %2 %3 %4 %5 %6 %7 %8 %9
echo ###########################################################################

for %%f in (%~dp0\test\*.wav) do (
	echo.
	echo ===========================================================================
	echo %%~nxf
	echo ===========================================================================
	echo.
	"%~dp0\bin\Win32\Release_Static\DynamicAudioNormalizerCLI.exe" -i "%%~f" -o "%~dp0\tmp\%%~nf.%1.wav" -l "%~dp0\tmp\%%~nf.%1.log" %2 %3 %4 %5 %6 %7 %8 %9
)

echo.
echo.

exit /b

:: ============================================================================

:done

echo Test Completed.
echo.
pause
