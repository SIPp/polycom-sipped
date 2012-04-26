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
    test = SippTest.new("test_bla_register_and_subscribe_two_lines_short_in_sequence", "-sf bla_register_and_subscribe_two_lines_short_client.sipp -mc", "-sf bla_register_and_subscribe_two_lines_short_in_sequence_server.sipp -mc")
    assert(test.run())
  end
  
  def test_bla_register_and_subscribe_two_lines_short_with_optional_client
    test = SippTest.new("test_bla_register_and_subscribe_two_lines_short_with_optional_client", "-sf bla_register_and_subscribe_two_lines_short_with_optional_client.sipp -mc", "-sf bla_register_and_subscribe_two_lines_short_in_sequence_server.sipp -mc")
    assert(test.run())
  end  

  # def test_bla_register_and_subscribe_two_lines_short_register_and_200_swapped_server_will_fail
    # test = SippTest.new("bla_register_and_subscribe_two_lines_short_in_sequence", "-sf bla_register_and_subscribe_two_lines_short_with_optional_client.sipp -mc", "-sf bla_register_and_subscribe_two_lines_short_register_and_200_swapped_server.sipp -mc")
    # test.expected_exitstatus = 123
    # test.expected_errormessage = something
    # assert(test.run())
  # end    

  def test_bla_register_and_subscribe_two_lines_short_register_and_200_swapped_server_will_succeed
    # will need to add parameter to enable (perhaps its on by default with -mc).
    test = SippTest.new("test_bla_register_and_subscribe_two_lines_short_register_and_200_swapped_server_will_succeed", "-sf bla_register_and_subscribe_two_lines_short_with_optional_client.sipp -mc", "-sf bla_register_and_subscribe_two_lines_short_register_and_200_swapped_server.sipp -mc")
    assert(test.run())
  end    
  

  
end

