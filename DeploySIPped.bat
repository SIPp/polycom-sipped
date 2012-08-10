dir
@echo ******* COMMITTING %SIPPED%\sipp.exe and %1 **************
echo %WORKSPACE%
echo %TA_DIR%
echo %SIPPED%
echo %Deploy%

cd "%TA_DIR%\SIPped\SIPped\src"
sipp.exe -v | grep SIP >> svncommitmessage.txt
echo ---commit message
type svncommitmessage.txt
echo ---
cd "%SIPPED%\..\Installation\SIPped"
dir 
"c:\Program Files\SlikSvn\bin\svn.exe" status
rem 
rem svn commit %1 "%SIPPED%\sipp.exe" --username jenkins  --password jenkins  --non-interactive -m "BugID(s): VOIP-64473\n\n%1 commit"
rem "c:\Program Files\SlikSvn\bin\svn.exe" commit %1 "%SIPPED%\sipp.exe" --username jenkins  --password jenkins  --non-interactive -F "%TA_DIR%\SIPped\SIPped\src\svncommitmessage.txt"
"c:\Program Files\SlikSvn\bin\svn.exe" commit %1 "%SIPPED%\sipp.exe" --non-interactive -F "%TA_DIR%\SIPped\SIPped\src\svncommitmessage.txt" --force-log

@echo archiving copy of msi file
pushd "\\vanfileprd\public\SIPped\Old SIPped Versions"
wmic logicaldisk get  name,description,providername
dir
if /I "%1"=="SIPped_windows7.msi" copy /y "%SIPPED%\..\Installation\SIPped_windows7*.msi" .
if /I "%1"=="SIPped.msi" copy /y "%SIPPED%\..\Installation\SIPped_v*.msi" .
popd 
rem
  