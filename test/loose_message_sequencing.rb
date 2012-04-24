#!/usr/bin/ruby
#
# Copyright (c) 2010, Polycom, Inc.
#
# Author: Edward Estabrook <edward.estabrook@polycom.com>
#


require 'test/unit'
require './sipp_test'

class LooseMessageSequencing < Test::Unit::TestCase
  def test_bla_register_and_subscribe_two_lines_short_in_sequence
    test = SippTest.new("bla_register_and_subscribe_two_lines_short_in_sequence", "-sf bla_register_and_subscribe_two_lines_short_client.sipp -mc", "-sf bla_register_and_subscribe_two_lines_short_in_sequence_server.sipp -mc")
    assert(test.run())
  end
  
  def test_bla_register_and_subscribe_two_lines_short_with_optional_client
    test = SippTest.new("bla_register_and_subscribe_two_lines_short_in_sequence", "-sf bla_register_and_subscribe_two_lines_short_with_optional_client.sipp -mc", "-sf bla_register_and_subscribe_two_lines_short_in_sequence_server.sipp -mc")
    assert(test.run())
  end  
end

