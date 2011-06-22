#!/usr/bin/ruby
#
# Copyright (c) 2010, Polycom, Inc.
#
# Author: Edward Estabrook <edward.estabrook@polycom.com>
#


require 'test/unit'
require './sipp_test'

class VerifyTestAction < Test::Unit::TestCase
  def test_test

    positive_test = SippTest.new("verify_test_action", "-sf verify_test_action_client.sipp -mc", "-sf verify_test_action_server.sipp -mc")
    assert(positive_test.run())
  end
  def test_test_fail
    negative_test = SippTest.new("verify_test_action_fail", "-sf verify_test_action_fail_client.sipp -mc", "-sf verify_test_action_fail_server.sipp -mc")
    negative_test.expected_error_log = /Test firstcseq ==  2 INVITE has failed/
    assert(negative_test.run())
  end
end

