REM 
REM	invoke ruby sipp test and copy xml results to unittest results directory
REM
REM

rake
echo "returned from rake"
xcopy /Y testresults\*.* ..\UnitTest\testresults
