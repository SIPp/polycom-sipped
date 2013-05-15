@echo Creating rsipp.exe...

pushd %SIPP_SOURCE%\rsipp\
call pp rsipp.pl -o rsipp.exe -M XML::SAX::PurePerl
copy /y rsipp.exe "%SIPP_SOURCE%\WindowsBinary\Installation\SIPp
del rsipp.exe
popd

popd

@echo Perl-based exe files created.
@echo .
@echo Building msi image...

rem  environ varibles must be set so path points to right version of sipp
rem  version being packaged is expected to be in %SIPP_SOURCE%\src
where sipp
rem extract sipp version number for use in installer
@setlocal
get_sipp_version_number.pl > sippversion.txt
set /p SIPP_VERSION= < sippversion.txt
@echo %SIPP_VERSION%

IF EXIST "%SIPP_SOURCE%\WindowsBinary\Installation\SIPp.msi" del "%SIPP_SOURCE%\WindowsBinary\Installation\SIPp.msi"
@echo Setting Installer version to %SIPP_VERSION%
"C:\Program Files (x86)\Caphyon\Advanced Installer 8.1.3\bin\x86\advinst.exe" /edit "%SIPP_SOURCE%\WindowsBinary\Installation\SIPp for Cygwin Setup.aip" /SetVersion %SIPP_VERSION% 
@echo Creating SIPp_cygwin.msi
"C:\Program Files (x86)\Caphyon\Advanced Installer 8.1.3\bin\x86\advinst.exe" /rebuild "%SIPP_SOURCE%\WindowsBinary\Installation\SIPp for Cygwin Setup.aip"
dir *.msi | grep SIPp
perl rename_sipp_msi_with_full_version.pl _cygwin
@echo Moving SIPp_cygwin.msi to SIPp folder for archive.
copy /y SIPp_cygwin.msi "%SIPP_SOURCE%\WindowsBinary\Installation\SIPp
del SIPp_cygwin.msi
@echo (SIPp_cygwin.msi and rsipp.exe are in the SIPp folder for check-in)
copy /y NUL "%SIPP_SOURCE%\WindowsBinary\Installation\SIPp\FOR WIN32 VERSION GOTO SIPP - WIN32 WORKSPACE, WIN32 NOT UPDATED HERE.txt"
@endlocal
