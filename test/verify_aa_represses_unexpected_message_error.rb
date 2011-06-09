#!/usr/bin/ruby
#
# Copyright (c) 2010, Polycom, Inc.
#
# Author: Edward Estabrook <edward.estabrook@polycom.com>
#


require 'test/unit'
require 'sipp_test'

class VerifyAARepressesUnexpedMessageError < Test::Unit::TestCase
  def test_aa_represses_unexpected_message_error
    # Test the functionality of -aa.  Client sends random message (since we only see errors from one side), the server then sends a register.
    # The client should automatically respond and then quit successfully (rather than taking the default action of throwing an error.
    test = SippTest.new("verify_aa_represses_unexpected_message_error", "-sf verify_aa_represses_unexpected_message_error_client.sipp -mc -aa", "-sf verify_aa_represses_unexpected_message_error_server.sipp -mc")
    assert(test.run())
  end
end

