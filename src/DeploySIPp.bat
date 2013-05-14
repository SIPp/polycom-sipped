dir
@echo ******* COMMITTING %SIPP%\sipp.exe and %1 **************
echo WORKSPACE = '%WORKSPACE%'
echo SIPP_SOURCE = '%SIPP_SOURCE%'
echo Deploy = '%Deploy%'

cd "%SIPP_SOURCE%\src"
sipp.exe -v | grep SIP >> jenkinscommitmessage.txt
echo ---commit message
type jenkinscommitmessage.txt
echo ---
cd "%SIPP_SOURCE%\WindowsBinary\Installation\SIPp"
dir 
git status
rem "c:\Program Files\SlikSvn\bin\svn.exe" commit %1 "%SIPP%\sipp.exe" --non-interactive -F "%SIPP_SOURCE\src\jenkinscommitmessage.txt" --force-log
rem git commit -a -F jenkinscommitmessage.txt
rem Maybe use copy of jenkinscommitmessage.txt for changing so it isn't tracked and we can just do git add -u
echo " Must add, commit and push sipp.exe, rsipp.exe and msi files (but not the jenkinscommitfile)."


@echo archiving copy of msi file
rem pushd "\\vanfileprd\public\SIPp\Old SIPp Versions"
pushd "%JENKINS_HOME%\userContent\Old SIPp Versions"
wmic logicaldisk get  name,description,providername
@echo copying to archive
if /I "%1"=="SIPp_windows7.msi" copy /y "%SIPP_SOURCE%\WindowsBinary\Installation\SIPp_windows7*.msi" .
if /I "%1"=="SIPp_cygwin.msi" copy /y "%SIPP_SOURCE%\WindowsBinary\Installation\SIPp_cygwin*.msi" .
dir
@echo updating current distribution copy
cd ..
if /I "%1"=="SIPp_windows7.msi" del SIPp_windows7*.msi
if /I "%1"=="SIPp_windows7.msi" del SIPp_v*.msi
if /I "%1"=="SIPp_windows7.msi" copy /y "%SIPP_SOURCE%\WindowsBinary\Installation\SIPp_windows7*.msi" .
if /I "%1"=="SIPp_cygwin.msi" copy /y "%SIPP_SOURCE%\WindowsBinary\Installation\SIPp_cygwin*.msi" .
dir
popd 

  