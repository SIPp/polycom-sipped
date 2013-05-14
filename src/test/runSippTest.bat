REM 
REM	invoke ruby sipp test and copy xml results to unittest results directory
REM assume invoke from windows running jenkins
REM


cd "%SIPP_SOURCE%\src\test"
REM  invoke tests through Rakefile.  pass verbose option through to each test
REM Older version of Rake required 'TESTOPTS="-- -v"'
rake TESTOPTS="-v"
echo "$?   returned from rake"
xcopy /Y testresults\*.* ..\UnitTest\testresults
