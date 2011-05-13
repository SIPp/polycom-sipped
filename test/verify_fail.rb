#!/usr/bin/ruby
#
# Copyright (c) 2010, Polycom, Inc.
#
# Author: Edward Estabrook <edward.estabrook@polycom.com>
#


require 'test/unit'
require 'sipp_test'

class Fail < Test::Unit::TestCase
  def test_failure
    # Verify failure
    test = SippTest.new("verify_failure", "-sf should_fail_client.sipp -mc", "-sf should_fail_server.sipp -mc")
    test.expected_error_log = /Transaction 'S1' in message 2 was initiated remotely and is therefore a server transaction. You can not use \[client_cseq\] with server transactions, use \[server_cseq\] instead.For most transactions you can use \[cseq\] and \[cseq_method\] and the correct value will be used../
    assert(test.run())
    #1. make sure error.log exists
    #2. read in error.log
    #3. string match it with expected.
#(note, this capability should probably be generalized in ruby framework for use by other tests)
	

  end
end


