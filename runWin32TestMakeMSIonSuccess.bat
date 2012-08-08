set TA_DIR=%WORKSPACE%\TestUtilities
set SIPPED=%TA_DIR%\SIPped\WindowsBinary\SIPped_win32
set PATH=%SIPPED%;%PATH%;%TA_DIR%\SIPped\rsipp

cd "%TA_DIR%\SIPped\SIPped\src\test"

REM "How busy is the server right now"
typeperf -sc 10 "\processor(_total)\%% processor time"

call runSippTest.bat CI-SIPPED-01_WIN32

echo Result of execution is: %ERRORLEVEL%

rem  if success copy binary into windowsbinary/sipped_win32 and build msi
IF %ErrorLevel% EQU 1 GOTO END
@echo copying sipp.exe to %SIPPED% 
xcopy /Y "%TA_DIR%\SIPped\SIPped\src\Debug\SIPped.exe" "%SIPPED%\sipp.exe"
copy /y NUL "%SIPPED%\..\SIPped\WARNING FILES HERE ARE NOT CURRENT.txt" > NUL
@setlocal
@set TMP=%PATH%
cd "%SIPPED%\..\Installation\"
call create_windows7_installer.bat
set PATH=%TMP%
@endlocal

IF %Deploy% EQU "No" GOTO :END:
rem DeploySIPpedWin32.bat

:END
exit /B %ERRORLEVEL%