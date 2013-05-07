#!/usr/bin/ruby
#
# Copyright (c) 2011, Polycom, Inc.
#
# Author: Daniel Busto <daniel.busto@polycom.com>
#


require 'test/unit'
require './sipp_test'

class ZeroContentLengthScenarios < Test::Unit::TestCase
  def test_zero_content_length_header_scenarios
    #Verify that content-length: [len] (with 0-length) will work in body.

    test_zero_content_length = SippTest.new("zero_content_length", "-sf zero_content_length_header_not_last_client.sipp -mc", "-sf zero_content_length_header_not_last_server.sipp -mc")

    assert(test_zero_content_length.run())

  end
end
