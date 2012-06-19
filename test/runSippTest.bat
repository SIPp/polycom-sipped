REM 
REM	invoke ruby sipp test and copy xml results to unittest results directory
REM assume invoke from windows running jenkins
REM

set TA_DIR=%WORKSPACE%\TestUtilities
set SIPPED=%WORKSPACE%\TestUtilities\SIPped\WindowsBinary\SIPped
set CYGWIN=nodosfilewarning

rem copy /Y  "%TA_DIR%\tmp\rsipp.config.xml" "%TA_DIR%\SIPped\rsipp\"
copy /Y  "%TA_DIR%\..\AutoTester\continuous_integration\TA_Config\CI-TA-01\SIPped\rsipp\rsipp.config.xml"  "%TA_DIR%\SIPped\rsipp\"

cat "%TA_DIR%\SIPped\rsipp\rsipp.config.xml"

cd "%TA_DIR%\SIPped\SIPped\src\test"
REM  invoke tests through Rakefile.  pass verbose option through to each test
rake TESTOPTS="-- -v"
echo "$?   returned from rake"
xcopy /Y testresults\*.* ..\UnitTest\testresults
