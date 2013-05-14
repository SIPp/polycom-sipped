rem Assume SIPP_SOURCE is set
set SIPP=%SIPP_SOURCE%\WindowsBinary\SIPp_cygwin
set CYGWIN=nodosfilewarning
set TERMINFO=%SIPP_SOURCE%\WindowsBinary\SIPp_cygwin\terminfo
set PATH=%SIPP%;%SIPP_SOURCE%\rsipp;%PATH%

cd "%SIPP_SOURCE%\src\test"

REM "How busy is the server right now"
typeperf -sc 10 "\processor(_total)\%% processor time"

call runSippTest.bat 

echo Result of execution is: %ERRORLEVEL%

rem  if success copy binary into windowsbinary/sipp and build msi
IF %ErrorLevel% EQU 1 GOTO END
@echo copying sipp.exe to %SIPP% 
xcopy /Y "%SIPP_SOURCE%\src\sipp.exe" "%SIPP%\sipp.exe"
copy /y NUL "%SIPP_SOURCE%\WindowsBinary\SIPp_win32\WARNING FILES HERE ARE NOT CURRENT.txt" > NUL
cd "%SIPP_SOURCE%\WindowsBinary\Installation\"
call create_cygwin_installer.bat

cd "%SIPP_SOURCE%\src"
IF /I "%Deploy%" NEQ "Yes" goto END
call DeploySIPp.bat SIPped_cygwin.msi

:END
exit /B %ERRORLEVEL%