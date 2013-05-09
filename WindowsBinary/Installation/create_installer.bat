@echo Creating rsipp.exe...

pushd %TA_DIR%\SIPped\rsipp\
call pp rsipp.pl -o rsipp.exe -M XML::SAX::PurePerl
mv --force rsipp.exe "%TA_DIR%\SIPped\WindowsBinary\Installation\SIPped
popd

@echo Creating snipp.exe...
pushd %TA_DIR%\SIPped\snipp\
call pp snipp.pl -o snipp.exe -M Params::ValidatePP -M Params::Validate::PP
mv --force snipp.exe "%TA_DIR%\SIPped\WindowsBinary\Installation\SIPped
popd

@echo Perl-based exe files created.
@echo .
@echo Building msi image...

rem  environ varibles must be set so path points to right version of sipp
rem  version being packaged is expected to be in %TA_DIR%\SIPped\SIPped\src
where sipp
rem extract sipp version number for use in installer
@setlocal
get_sipp_version_number.pl > sippversion.txt
set /p SIPP_VERSION= < sippversion.txt
@echo %SIPP_VERSION%

IF EXIST "%TA_DIR%\SIPped\WindowsBinary\Installation\SIPped.msi" del "%TA_DIR%\SIPped\WindowsBinary\Installation\SIPped.msi"
@echo Setting Installer version to %SIPP_VERSION%
"C:\Program Files (x86)\Caphyon\Advanced Installer 8.1.3\bin\x86\advinst.exe" /edit "%TA_DIR%\SIPped\WindowsBinary\Installation\SIPped Installation Setup.aip" /SetVersion %SIPP_VERSION% 
@echo Creating SIPped.msi
"C:\Program Files (x86)\Caphyon\Advanced Installer 8.1.3\bin\x86\advinst.exe" /rebuild "%TA_DIR%\SIPped\WindowsBinary\Installation\SIPped Installation Setup.aip"
dir *.msi | grep SIPped
perl rename_sipp_msi_with_full_version.pl
@echo Moving SIPped.msi to SIPped folder for archive.
@mv --force SIPped.msi "%TA_DIR%\SIPped\WindowsBinary\Installation\SIPped
@echo (SIPped.msi, snipp.exe and rsipp.exe are in the SIPped folder for check-in)
copy /y NUL "%TA_DIR%\SIPped\WindowsBinary\Installation\SIPped\FOR WIN32 VERSION GOTO SIPPED - WIN32 WORKSPACE, WIN32 NOT UPDATED HERE.txt"
@endlocal
