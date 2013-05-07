#!/usr/bin/ruby
#
# Copyright (c) 2010, Polycom, Inc.
#
# Author: Edward Estabrook <edward.estabrook@polycom.com>
#


require 'test/unit'
require './sipp_test'

class VerifySLL < Test::Unit::TestCase
  def test_cooked_packets
    test = SippTest.new("cooked_packets", "-sf Cooked_Packets_client.sipp -mc", "-sf Cooked_Packets_server.sipp -mc")
    assert(test.run())
  end
end

