#!/usr/bin/ruby
#
# Copyright (c) 2010, Polycom, Inc.
#
# Author: Edward Estabrook <edward.estabrook@polycom.com>
#


require 'test/unit'
require './sipp_test'

class ErrorMessageTests < Test::Unit::TestCase
  def test_verify_transaction_failure
    # Verify failure
    server_transaction_test = SippTest.new("verify_transaction_failure", "-sf transaction_fail_client.sipp -mc", "-sf transaction_fail_server.sipp -mc")
    server_transaction_test.expected_error_log = /Transaction 'S1' in message 2 was initiated remotely and is therefore a server transaction. You can not use \[client_cseq\] with server transactions, use \[server_cseq\] instead.For most transactions you can use \[cseq\] and \[cseq_method\] and the correct value will be used../
    server_transaction_test.expected_exitstatus = 255
    assert(server_transaction_test.run())
  end

  def test_verify_to_double_failure
    to_double_test = SippTest.new("verify_to_double_failure", "-sf to_double_fail_client.sipp -mc", "-sf to_double_fail_server.sipp -mc")
    to_double_test.expected_error_log = /Invalid format. Could not convert "INVITE" to a double, while performing add./
    to_double_test.expected_exitstatus = 255
    assert(to_double_test.run())
  end
end


