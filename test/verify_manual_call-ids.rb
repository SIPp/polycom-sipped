#!/usr/bin/ruby
#
# Copyright (c) 2011, Polycom, Inc.
#
# Author: Daniel Busto <daniel.busto@polycom.com>
#


require 'test/unit'
require './sipp_test'

class KeywordScenarios < Test::Unit::TestCase
  def test_manual_call_id
    #Verify CSeq and branch

    test_manual_call_id = SippTest.new("manual_call_id", "-sf manual_call-id_client.sipp -mc", "-sf manual_call-id_server.sipp -mc")
    assert(test_manual_call_id.run())

  end
end
