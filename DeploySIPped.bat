dir
@echo ******* COMMITTING sipp.exe and %1 **************
echo %WORKSPACE%
echo %TA_DIR%
echo %SIPPED%
echo %Deploy%
cd "%TA_DIR%"
"c:\Program Files\SlikSvn\bin\svn.exe" info

cd "%SIPPED%\src\SIPped\SIPped\src"
sipp.exe -v | grep SIP >> svncommitmessage.txt
cd "%SIPPED%\..\Installation\SIPped"
dir 
"c:\Program Files\SlikSvn\bin\svn.exe" status
rem 
rem svn commit %1 "%SIPPED%\sipp.exe" --username jenkins  --password jenkins  --non-interactive -m "BugID(s): VOIP-64473\n\n%1 commit"
rem "c:\Program Files\SlikSvn\bin\svn.exe" commit %1 "%SIPPED%\sipp.exe" --username jenkins  --password jenkins  --non-interactive -F "%TA_DIR%\SIPped\SIPped\src\svncommitmessage.txt"
svn commit %1 --non-interactive -F "%TA_DIR%\SIPped\SIPped\src\svncommitmessage.txt" --force-log
rem
  