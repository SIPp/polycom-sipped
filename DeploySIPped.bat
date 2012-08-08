dir
echo %WORKSPACE%
echo %SIPPED%
echo %Deploy%

cd "%SIPPED%\..\Installation\SIPped"
dir 
rem 
rem svn commit %1  --username jenkins  --password jenkins  --non-interactive -m "BugID(s): VOIP-64473\n\n%1 commit"
rem
cd "%SIPPED%"
rem 
rem svn commit sipp.exe  --username jenkins  --password jenkins  --non-interactive -m "BugID(s): VOIP-64473\n\nsipp.exe commit"
rem 
  