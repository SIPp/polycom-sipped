@ECHO OFF
setlocal

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
REM IF (%argC%) EQU (0) (set pathname=%CD%) 
IF (%argC%) EQU (0) (set pathname="`pwd`") 
echo %pathname%

REM PATH to make, rm, and other makefile commands
c:\cygwin\bin\bash   -c 'export PATH="/usr/local/bin:/usr/bin:${PATH}";cd %pathname%;make clean;'



endlocal