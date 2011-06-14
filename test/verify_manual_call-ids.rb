#!/usr/bin/ruby
#
# Copyright (c) 2011, Polycom, Inc.
#
# Author: Daniel Busto <daniel.busto@polycom.com>
#


require 'test/unit'
require './sipp_test'

class KeywordScenarios < Test::Unit::TestCase
  def test_scenarios
    #Verify CSeq and branch

    test_reject = SippTest.new("reject_call", "-sf manual_call-id_client.sipp -mc", "-sf manual_call-id_server.sipp -mc")
    assert(test_reject.run())

  end
end
