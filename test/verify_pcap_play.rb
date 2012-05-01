#!/usr/bin/ruby
#
# Copyright (c) 2010, Polycom, Inc.
#
# Author: Edward Estabrook <edward.estabrook@polycom.com>
#


require 'test/unit'
require './sipp_test'

class VerifyPcapPlay < Test::Unit::TestCase
  def test_pcap_play
    #check all charecters are encoded properly (except ';', '[' and ']' which cause errors for SIPp)
    test = SippTest.new("verify_pcap_play", "-sf verify_pcap_play_client.sipp -mc", "-sf verify_pcap_play_server.sipp -mc")
    assert(test.run())
  end
  
  def test_pcap_play_fails_when_no_media
    test = SippTest.new("test_pcap_play_fails_when_no_media", "-sf verify_pcap_fails_when_no_media_client.sipp -mc", "-sf verify_pcap_fails_when_no_media_server.sipp -mc")
    test.expected_exitstatus=255
    assert(test.run())
  end
  
end

