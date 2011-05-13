#!/usr/bin/ruby
#
# Copyright (c) 2011, Polycom, Inc.
#
# Author: Daniel Busto <daniel.busto@polycom.com>
#


require 'test/unit'
require 'sipp_test'

class KeywordScenarios < Test::Unit::TestCase
  def test_scenarios
    #Verify CSeq and branch

    test_reject = SippTest.new("reject_call", "-sf reject_call_client.sipp -mc", "-sf reject_call_server.sipp -mc")
    assert(test_reject.run())

    test_conference = SippTest.new("conference_call", "-sf conference_call_client.sipp -mc", "-sf conference_call_server.sipp -mc")
    assert(test_conference.run())

    test_place = SippTest.new("place_call", "-sf place_call_client.sipp -mc", "-sf place_call_server.sipp -mc")
    assert(test_place.run())

    test_receive = SippTest.new("receive_call", "-sf receive_call_client.sipp -mc", "-sf receive_call_server.sipp -mc")
    assert(test_receive.run())
  end
end
