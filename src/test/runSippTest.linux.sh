#!/bin/sh
#
# Must have SIPP_SOURCE set to root directory
#
#TERM must be defined or sipp will fail to launch, should this be login profile
export TERM=xterm

#how do we alter checkin so this is executable on checkout
ls -la  "$SIPP_SOURCE/rsipp/rsipp.pl"
# exec.rb test case requires exec rsipp.pl to be able to find rsipp.pl as an executable.
chmod 755 "$SIPP_SOURCE/rsipp/rsipp.pl"
export PATH=$PATH:$TA_DIR/SIPped/rsipp
echo "PATH = $PATH"

cd "$SIPP_SOURCE/src/test"
rake
echo "rake exit code: $?"
cp testresults/*.xml ../UnitTest/testresults/

exit 0

#reference, the following tests usutally fail if there are env variable problems
# 
#ruby verify_whereami.rb   --name=test_unset_env -- -v
#ruby exec.rb --name=test_exec_uses_correct_path -- -v

