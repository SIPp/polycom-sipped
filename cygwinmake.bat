@ECHO OFF
setlocal

set TA_DIR=%WORKSPACE%\TestUtilities
set SIPPED=%WORKSPACE%\TestUtilities\SIPped\WindowsBinary\SIPped
set CYGWIN=nodosfilewarning

if not exist c:\cygwin (
  echo "Can't find c:\cygwin, stopping"
  GOTO:EOF)
  

if exist C:\TA_Config\SIPped\rsipp\rsipp.config.xml (
  copy /Y  C:\TA_Config\SIPped\rsipp\rsipp.config.xml "%TA_DIR%\SIPped\rsipp\" ) 
  
cd "%TA_DIR%\SIPped\SIPped\src"

IF  /I "X%1"=="Xhelp" goto :HELP
IF  /I "X%1"=="X?" goto :HELP
goto:EXECUTE
:HELP
(
	ECHO "usage: %0 [directory]"
	ECHO "       if [directory] is not supplied assumes current directory is target"
	ECHO "       directory specification must use forward slash "
	ECHO "       eg.  cywinmake z:/SIPped/src"
	GOTO:EOF
)


:EXECUTE
REM get count of command line args
set /A argC=0
for %%x in (%*) do Set /A argC+=1

REM if no args use current directory, else first arg is full path using fwd slash
set pathname=%1
REM  %CD% yields backslashes which shell does not accept as directory delimiters, need dos version of sed to change slashes before sending to bash
REM IF (%argC%) EQU (0) (set pathname=%CD%) 
IF (%argC%) EQU (0) (set pathname="`pwd`") 

REM PATH to make, rm, and other makefile commands
c:\cygwin\bin\bash   -c 'PATH="/usr/local/bin:/usr/bin:${PATH}";export PATH;cd %pathname%;make debug_pcap_ossl_cygwin; make unit_test;'


REM --rcfile doesnt seem to process profile, no change to path 
REM c:\cygwin\bin\bash  --rcfile /etc/profile -c 'echo $PATH;cd %pathname%;make debug_pcap_ossl_cygwin; make unit_test;'

REM --login gets us full profile but we get put into home directory, lose current working directory 
REM c:\cygwin\bin\bash  --login -c 'echo $PATH;cd %pathname%;make debug_pcap_ossl_cygwin; make unit_test;'
endlocal
