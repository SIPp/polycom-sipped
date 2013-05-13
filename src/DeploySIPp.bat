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
cd "%SIPP_SOURCE%\WindowsBinary\Installation\SIPped"
dir 
git status
rem "c:\Program Files\SlikSvn\bin\svn.exe" commit %1 "%SIPP%\sipp.exe" --non-interactive -F "%SIPP_SOURCE\src\jenkinscommitmessage.txt" --force-log
rem git commit -am -F jenkinscommitmessage.txt
rem Maybe use copy of jenkinscommitmessage.txt for changing so it isn't tracked and we can just do git add -u
echo " Must add, commit and push sipp.exe, rsipp.exe and msi files (but not the jenkinscommitfile)."


@echo archiving copy of msi file
rem pushd "\\vanfileprd\public\SIPped\Old SIPped Versions"
pushd "%JENKINS_HOME%\userContent\Old SIPp Versions"
wmic logicaldisk get  name,description,providername
@echo copying to archive
if /I "%1"=="SIPped_windows7.msi" copy /y "%SIPP_SOURCE%\WindowsBinary\Installation\SIPped_windows7*.msi" .
if /I "%1"=="SIPped_cygwin.msi" copy /y "%SIPP_SOURCE%\WindowsBinary\Installation\SIPped_v*.msi" .
dir
@echo updating current distribution copy
cd ..
if /I "%1"=="SIPped_windows7.msi" del SIPped_windows7*.msi
if /I "%1"=="SIPped_windows7.msi" del SIPped_v*.msi
if /I "%1"=="SIPped_windows7.msi" copy /y "%SIPP_SOURCE%\WindowsBinary\Installation\SIPped_windows7*.msi" .
if /I "%1"=="SIPped_cygwin.msi" copy /y "%SIPP_SOURCE%\WindowsBinary\Installation\SIPped_cygwin*.msi" .
dir
popd 

  