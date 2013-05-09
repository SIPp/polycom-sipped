rem ******************************************************************
rem *   20120731  compile windows port of sipp
rem ******************************************************************

rem Set the environment for msbuild
call "C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\vcvarsall.bat" x86

set SIPP=%SIPP_SOURCE%\WindowsBinary\SIPp_win32
rem set INCLUDE=C:\Program Files\Microsoft SDKs\Windows\v6.0A\Include
rem set LIB=C:\Program Files\Microsoft SDKs\Windows\v6.0A\Lib
@echo "*********************************************"
cd "%SIPP_SOURCE%\src"

del Debug\*.exe sipp.exe

msbuild SIPp.sln /t:rebuild
IF %ErrorLevel% NEQ 0 GOTO END

rem put executable into src directory as expected by system tests
copy /Y Debug\SIPp.exe sipp.exe
set PATH=%PATH%;%SIPP%
sipp.exe -v

rem run unittests
cd UnitTest
..\Debug\SIPpTest.exe --gtest_output=xml:testresults/SIPpTest.xml 
cd ..

:END
exit /B %ERRORLEVEL%