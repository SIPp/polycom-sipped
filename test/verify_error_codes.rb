#!/usr/bin/ruby
#
# Copyright (c) 2010, Polycom, Inc.
#
# Author: Daniel Busto <daniel.busto@polycom.com>
#

require 'test/unit'
require './sipp_test'

class ErrorCodes < Test::Unit::TestCase
  def test_fatal
    test = SippTest.new("test_fatal_error_code", "-sf verify_fatal_error_code_client.sipp -mc", "-sf verify_fatal_error_code_server.sipp -mc")
    test.expected_exitstatus = 255
    assert(test.run())
  end
  def test_fail  
    test = SippTest.new("test_fail_error_code", "-sf verify_fail_error_code_client.sipp -mc", "-sf verify_fail_error_code_server.sipp -mc")
    test.expected_exitstatus = 1
    assert(test.run())
  end
end
