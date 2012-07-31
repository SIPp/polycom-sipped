rem ******************************************************************
rem *   20120731  compile windows port of sipped
rem ******************************************************************

rem Set the environment for msbuild
"C:\Program Files (x86)\Microsoft Visual Studio 8\VC\vcvarsall.bat"
msbuild SIPped.sln /t:rebuild

rem put executable into src directory
copy /Y Debug\SIPped.exe sipp.exe
set PATH=%PATH%;%WORKSPACE%\TestUtilities\SIPped\WindowsBinary\SIPped_win32
sipp.exe -v

rem run unittests
cd UnitTest
..\Debug\SIPpedTest.exe
cd ..

