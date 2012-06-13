REM 
REM	invoke ruby sipp test and copy xml results to unittest results directory
REM assume invoke from windows running jenkins
REM

set TA_DIR=%WORKSPACE%\TestUtilities
set SIPPED=%WORKSPACE%\TestUtilities\SIPped\WindowsBinary\SIPped
set CYGWIN=nodosfilewarning

rem copy /Y  C:\TA_Config\SIPped\rsipp\rsipp.config.xml "%TA_DIR%\SIPped\rsipp\"
copy /Y  c:\TA_config\CI-TA-01\SIPped\rsipp\rsipp.config.xml "%TA_DIR%\SIPped\rsipp\"

cat "%TA_DIR%\SIPped\rsipp\rsipp.config.xml"

cd "%TA_DIR%\SIPped\SIPped\src\test"
rake
echo "$?   returned from rake"
xcopy /Y testresults\*.* ..\UnitTest\testresults
