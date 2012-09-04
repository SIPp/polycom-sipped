set TA_DIR=%WORKSPACE%\TestUtilities
set SIPPED=%WORKSPACE%\TestUtilities\SIPped\WindowsBinary\SIPped
set CYGWIN=nodosfilewarning
set TERMINFO=%TA_DIR%\SIPped\WindowsBinary\SIPped\terminfo
set PATH=%SIPPED%;%PATH%;%TA_DIR%\SIPped\rsipp

cd "%TA_DIR%\SIPped\SIPped\src\test"

REM "How busy is the server right now"
typeperf -sc 10 "\processor(_total)\%% processor time"

call runSippTest.bat CI-SIPPED-01_CYGWIN

echo Result of execution is: %ERRORLEVEL%

rem  if success copy binary into windowsbinary/sipped and build msi
IF %ErrorLevel% EQU 1 GOTO END
@echo copying sipp.exe to %SIPPED% 
xcopy /Y "%TA_DIR%\SIPped\SIPped\src\sipp.exe" "%SIPPED%\sipp.exe"
copy /y NUL "%SIPPED%\..\SIPped_win32\WARNING FILES HERE ARE NOT CURRENT.txt" > NUL
cd "%SIPPED%\..\Installation\"
call create_installer.bat

cd "%TA_DIR%\SIPped\SIPped\src"
IF /I "%eploy%"=="Yes" call DeploySIPped.bat SIPped.msi

:END
exit /B %ERRORLEVEL%