rem ******************************************************************
rem *   20120731  compile windows port of sipped
rem ******************************************************************

rem Set the environment for msbuild
call "C:\Program Files (x86)\Microsoft Visual Studio 8\VC\vcvarsall.bat" x86

set TA_DIR=%WORKSPACE%\TestUtilities
set SIPPED=%WORKSPACE%\TestUtilities\SIPped\WindowsBinary\SIPped
rem set INCLUDE=C:\Program Files\Microsoft SDKs\Windows\v6.0A\Include
rem set LIB=C:\Program Files\Microsoft SDKs\Windows\v6.0A\Lib
echo "*********************************************"


msbuild SIPped.sln /t:rebuild

rem put executable into src directory
copy /Y Debug\SIPped.exe sipp.exe
set PATH=%PATH%;%WORKSPACE%\TestUtilities\SIPped\WindowsBinary\SIPped_win32
sipp.exe -v

rem run unittests
cd UnitTest
..\Debug\SIPpedTest.exe
cd ..

