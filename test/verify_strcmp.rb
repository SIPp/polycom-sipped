#!/usr/bin/ruby
#
# Copyright (c) 2010, Polycom, Inc.
#
# Author: Edward Estabrook <edward.estabrook@polycom.com>
#


require 'test/unit'
require './sipp_test'

class VerifyStrcmp < Test::Unit::TestCase
  def test_test
    positive_test = SippTest.new("verify_strcmp", "-sf verify_strcmp_client.sipp -mc -trace_debug", "-sf verify_strcmp_server.sipp -mc")
    assert(positive_test.run())
  end
  def test_test_fail
    negative_test = SippTest.new("verify_strcmp_fail", "-sf verify_strcmp_fail_client.sipp -mc", "-sf verify_strcmp_fail_server.sipp -mc")
    negative_test.expected_error_log = /String comparision between variables firstcseq and secondcseq has failed, because INVITE is NOT the same as INV/
    assert(negative_test.run())
  end
end

