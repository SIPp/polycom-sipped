@echo Creating rsipp.exe...

pushd %SIPP_SOURCE%\rsipp\
call pp rsipp.pl -o rsipp.exe -M XML::SAX::PurePerl
copy /y rsipp.exe "%SIPP_SOURCE%\WindowsBinary\Installation\SIPp
del rsipp.exe
popd

@echo Perl-based exe files created.
@echo .
@echo Building msi image...

rem  environ varibles must be set so path points to right version of sipp
rem  version being packaged is expected to be in %SIPP_SOURCE%\src
rem  this is done automatically by win32make.bat
where sipp

rem extract sipp version number for use in installer
@setlocal
get_sipp_version_number.pl > sippversion.txt

set /p SIPP_VERSION= < sippversion.txt
@echo %SIPP_VERSION%

IF EXIST "%SIPP_SOURCE%\WindowsBinary\Installation\SIPp_windows7.msi" del "%SIPP_SOURCE%\WindowsBinary\Installation\SIPp_windows7.msi"
@echo Setting Installer version to %SIPP_VERSION%
"C:\Program Files (x86)\Caphyon\Advanced Installer 8.1.3\bin\x86\advinst.exe" /edit "%SIPP_SOURCE%\WindowsBinary\Installation\SIPp for Windows 7 Setup.aip" /SetVersion %SIPP_VERSION%  
@echo Creating SIPp_windows7.msi
"C:\Program Files (x86)\Caphyon\Advanced Installer 8.1.3\bin\x86\advinst.exe" /rebuild "%SIPP_SOURCE%\WindowsBinary\Installation\SIPp for Windows 7 Setup.aip"
dir *.msi | grep SIPp
rem argument must match portion of msi filename used to indicate win32 version, eg _windows7 for SIPp_windows7.msi 
perl rename_sipp_msi_with_full_version.pl _windows7
@echo Moving SIPp_windows7.msi to SIPp folder for archive.
copy /y SIPp_windows7.msi "%SIPP_SOURCE%\WindowsBinary\Installation\SIPp
del SIPp_windows7.msi
@echo (SIPp_windows7.msi and rsipp are in the SIPp folder for check-in)
copy /y NUL "%SIPP_SOURCE%\WindowsBinary\Installation\SIPp\FOR CYGWIN VERSION GOTO SIPP - CYGWIN WORKSPACE, CYGWIN NOT UPDATED HERE.txt"
@endlocal