#!/bin/sh
#
# assumes Jenkins will run this script
# 
export SIPPED="$WORKSPACE/TestUtilities/SIPped/WindowsBinary/SIPped"
#TERM must be defined or sipp will fail to launch, should this be login profile
export TERM=xterm

# reference rsipp.config.xml must exist on test platform 
cp /home/jenkins/TA_Config/TestUtilities/SIPped/rsipp/rsipp.config.xml "$TA_DIR/SIPped/rsipp"
cat "$TA_DIR/SIPped/rsipp/rsipp.config.xml"

#how do we alter svn checkin so this is executable on checkout
ls -la  "$TA_DIR/SIPped/rsipp/rsipp.pl"
# exec.rb test case requires exec rsipp.pl to be able to find rsipp.pl as an executable.
chmod 755 "$TA_DIR/SIPped/rsipp/rsipp.pl"
export PATH=$PATH:$TA_DIR/SIPped/rsipp
echo "PATH = $PATH"

cd "$TA_DIR/SIPped/SIPped/src/test"
rake
echo "rake exit code: $?"
cp testresults/*.xml ../UnitTest/testresults/

exit 0

#reference, the following tests usutally fail if there are env variable problems
# 
#ruby verify_whereami.rb   --name=test_unset_env -- -v
#ruby exec.rb --name=test_exec_uses_correct_path -- -v

