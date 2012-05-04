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
    # messages delivered in order, no optional messages.
    test = SippTest.new("test_bla_register_and_subscribe_two_lines_short_in_sequence", "-sf bla_register_and_subscribe_two_lines_short_client.sipp -mc", "-sf bla_register_and_subscribe_two_lines_short_in_sequence_server.sipp -mc")
    assert(test.run())
  end
  
  def test_bla_register_and_subscribe_two_lines_short_with_optional_client_messages_that_are_never_sent
    test = SippTest.new("test_bla_register_and_subscribe_two_lines_short_with_optional_client_messages_that_are_never_sent", "-sf bla_register_and_subscribe_two_lines_short_with_optional_client.sipp -mc", "-sf bla_register_and_subscribe_two_lines_short_in_sequence_server.sipp -mc")
    assert(test.run())
  end  


  def test_bla_register_and_subscribe_two_lines_short_with_optional_client_messages_that_are_sent_several_times
    test = SippTest.new("test_bla_register_and_subscribe_two_lines_short_with_optional_client_messages_that_are_sent_several_times", "-sf bla_register_and_subscribe_two_lines_short_with_optional_client.sipp -mc", "-sf bla_register_and_subscribe_two_lines_short_in_sequence_server_optional_messages_sent_server.sipp -mc")
    assert(test.run())
  end  


  def test_bla_register_and_subscribe_two_lines_short_register_and_200_swapped_server_will_succeed
    test = SippTest.new("test_bla_register_and_subscribe_two_lines_short_register_and_200_swapped_server_will_succeed", "-sf bla_register_and_subscribe_two_lines_short_with_optional_client.sipp -mc", "-sf bla_register_and_subscribe_two_lines_short_register_and_200_swapped_server.sipp -mc")
    assert(test.run())
  end    
  
 
  def test_bla_register_and_subscribe_two_lines_short_register_and_200_swapped_with_optional_server_will_succeed
    test = SippTest.new("test_bla_register_and_subscribe_two_lines_short_register_and_200_swapped_with_optional_server_will_succeed", "-sf bla_register_and_subscribe_two_lines_short_with_optional_client.sipp -mc", "-sf bla_register_and_subscribe_two_lines_short_register_and_200_swapped_with_optional_server.sipp -mc")
    assert(test.run())
  end    

  
# negative tests here...

  def test_bla_register_and_subscribe_two_lines_short_register_and_200_swapped_with_optional_too_early_will_fail
    test = SippTest.new("test_bla_register_and_subscribe_two_lines_short_register_and_200_swapped_with_optional_too_early_will_fail", "-sf bla_register_and_subscribe_two_lines_short_with_optional_client.sipp -mc", "-sf bla_register_and_subscribe_two_lines_short_register_and_200_swapped_with_optional_too_early_server.sipp -mc")
    test.expected_exitstatus = 1
    test.expected_error_log = /unexpected message.*while expecting \'RANDOM\' \(index 3\)\. Request \'RANDOM\' does not match expected request \'NOTIFY\'/
    assert(test.run())
  end    
  

    
  # client has next branch which forces loose_message_sequence=false for the client
  # using swapped server should force client to fail with unexpected message
  def test_bla_register_and_subscribe_two_lines_short_register_and_200_swapped_server_w_nextclient
    test = SippTest.new("test_bla_register_and_subscribe_two_lines_short_register_and_200_swapped_server_w_nextclient", "-sf bla_register_and_subscribe_two_lines_short_with_optional_client_next.sipp -mc", "-sf bla_register_and_subscribe_two_lines_short_register_and_200_swapped_server.sipp -mc")
    test.expected_exitstatus = 255
    test.expected_error_log = /unexpected message.*while expecting \'RANDOM\'\s\(index 3\)\.\sResponse\s\'200\'\sdoes not match/
    assert(test.run())
  end    
  
  # client has jump branch which forces loose_message_sequence=false for the client
  # using swapped server should force client to fail with unexpected message
  def test_bla_register_and_subscribe_two_lines_short_register_and_200_swapped_server_w_jumpclient
    test = SippTest.new("test_bla_register_and_subscribe_two_lines_short_register_and_200_swapped_server_w_jumpclient", "-sf bla_register_and_subscribe_two_lines_short_with_optional_client_jump.sipp -mc", "-sf bla_register_and_subscribe_two_lines_short_register_and_200_swapped_server.sipp -mc")
    test.expected_exitstatus = 255
    test.expected_error_log = /unexpected message.*while expecting \'RANDOM\'\s\(index 3\)\.\sResponse\s\'200\'\sdoes not match/
    assert(test.run())
  end    
end

