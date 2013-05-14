set SIPP=%SIPP_SOURCE%\WindowsBinary\SIPp_win32
set PATH=%SIPP%;%SIPP_SOURCE%\rsipp;%PATH%

cd "%SIPP_SOURCE%\src\test"

REM "How busy is the server right now"
typeperf -sc 10 "\processor(_total)\%% processor time"

call runSippTest.bat

echo Result of execution is: %ERRORLEVEL%

rem  if success copy binary into windowsbinary/sipp_win32 and build msi
IF %ErrorLevel% EQU 1 GOTO END
@echo copying sipp.exe to %SIPP% for use in msi creation. 
xcopy /Y "%SIPP_SOURCE%\src\Debug\SIPp.exe" "%SIPP%\sipp.exe"
copy /y NUL "%SIPP_SOURCE%\WindowsBinary\SIPp\WARNING FILES HERE ARE NOT CURRENT.txt" > NUL
@setlocal
@set TMP=%PATH%
cd "%SIPP_SOURCE%\WindowsBinary\Installation\"
call create_windows7_installer.bat
set PATH=%TMP%
@endlocal

cd "%SIPP_SOURCE%\src"
IF /I "%Deploy%" NEQ "Yes" GOTO END
call DeploySIPp.bat SIPp_windows7.msi

:END
exit /B %ERRORLEVEL%