#!/usr/bin/ruby
#
# Copyright (c) 2012, Polycom, Inc.
#
# Author: me :>
#


require 'test/unit'
require './sipp_test'

class Retransmits < Test::Unit::TestCase
  # normal uas uac 
  def test_uas_uac_reference
    test = SippTest.new("test_uas_uac_reference", "-sf myuac.sipp -mc", "-sf myuas.sipp -mc ")
    #test.expected_exitstatus = 255
    #test.expected_error_log = /Optional message 5\(4\) followed by message 7, a SEND message which is not allowed/
    assert(test.run())
  end    
  
  #to server: retransmit invite immediately
  def test_uas_uac_rrxmt_invite1
    test = SippTest.new("test_uas_uac_rrxmt_invite1", "-sf myuac_rxmt_invite1.sipp -mc", "-sf myuas.sipp -mc -ar")
    assert(test.run())
  end    
  
  #presence of -yr should not change behavoir
  #to server: retransmit invite immediately
  def test_uas_uac_rrxmt_invite1_yr
    test = SippTest.new("test_uas_uac_rrxmt_invite1_yr", "-sf myuac_rxmt_invite1.sipp -mc", "-sf myuas.sipp -mc -ar -yr")
    assert(test.run())
  end    
  
  #-mc should automatically enable -ar functionality
  #to server: retransmit invite immediately
  def test_uas_uac_rrxmt_invite1_mc
    test = SippTest.new("test_uas_uac_rrxmt_invite1_mc", "-sf myuac_rxmt_invite1.sipp -mc", "-sf myuas.sipp -mc")
    assert(test.run())
  end    
  
  
  #to server: retransmit invite twice immediately
  def test_uas_uac_rrxmt_invite2
    test = SippTest.new("test_uas_uac_rrxmt_invite2", "-sf myuac_rxmt_invite2.sipp -mc", "-sf myuas.sipp -mc -ar")
    assert(test.run())
  end    
  #presence of -yr should not change behavoir
  #to server: retransmit invite twice immediately
  def test_uas_uac_rrxmt_invite2
    test = SippTest.new("test_uas_uac_rrxmt_invite2", "-sf myuac_rxmt_invite2.sipp -mc", "-sf myuas.sipp -mc -ar")
    assert(test.run())
  end    
  
  #to server: retransmit invite twice - second invite delayed until after rx 200 ok
  def test_uas_uac_rrxmt_invite2_delay2ndInvite
    test = SippTest.new("test_uas_uac_rrxmt_invite2_delay2ndInvite", "-sf myuac_rxmt_invite2_delay2ndInvite.sipp -mc", "-sf myuas.sipp -mc -ar")
    assert(test.run())
  end    
  
  #retransmit invite twice - first invite delayed until after rx 200 ok, second after sending ACK
  #  emulates a out of sequence due to network delay of last invite.
  def test_uas_uac_rrxmt_invite2_delay2_invites
    test = SippTest.new("test_uas_uac_rrxmt_invite2_delay2_invites", "-sf myuac_rxmt_invite2_delay2_invites.sipp -mc", "-sf myuas.sipp -mc -ar")
    assert(test.run())
  end    
  
########################
# these are from loose_message_sequencing negative tests that now pass
# because we introduce this accept retransmitted messages feature.
# these are retransmisions to client
    
# 0 :      INVITE(1 ) <----------    0 :      INVITE(1 ) ---------->
#                                    1 :         100(1 ) <*---------
# 1 :         180(1 ) ---------->    2 :         180(1 ) <*---------
#                                    3 :         183(1 ) <*---------
# 2 :         200(1 ) ---------->    4 :         200(1 ) <----------
# 3 :         200(1 ) ---------->    
# 4 :         ACK(1 ) <*---------    5 :         ACK(1 ) ---------->
#                                    6 :       Pause     [   1000ms]
# 5 :         BYE(1 ) <----------    7 :         BYE(1 ) ---------->
# 6 :         200(1 ) ---------->    8 :         200(1 ) <----------

  
  def test_unexpected_message_during_pause
    test = SippTest.new("test_unexpected_message_during_pause", "-sf myuac_dialog_1secPause.sipp -mc -ar", "-sf myuas_dialog_double200.sipp  -mc ")
    #test.expected_exitstatus = 1
    #test.expected_error_log = /Aborting call on unexpected message for.*while pausing \(index 6\). Response \'200\' does not match.*Message index   7,\s*BYE\(1\)/m
    assert(test.run())
  end 
  
 #          SERVER                          CLIENT  
# 0 :      INVITE(1 ) <----------    0 :      INVITE(1 ) ---------->  
#                                    1 :         100(1 ) <*---------  
# 1 :         180(1 ) ---------->    2 :         180(1 ) <*---------  
#                                    3 :         183(1 ) <*---------  
# 2 :         200(1 ) ---------->    4 :         200(1 ) <----------  
# 3 :         200(1 ) ---------->                                       should fail here
# 4 :         ACK(1 ) <*---------    5 :         ACK(1 ) ---------->  
#                                    6 :       Pause     [   1000ms]  
# 5 :         BYE(1 ) <----------    7 :         BYE(1 ) ---------->  
# 6 :         200(1 ) ---------->    8 :         200(1 ) <----------  
      
# 7 :      INVITE(2 ) <----------    9 :      INVITE(2 ) ---------->  
#                                    10:         100(2 ) <*---------  
# 8 :         180(2 ) ---------->    11:         180(2 ) <*---------  
#                                    12:         183(2 ) <*---------  
# 9 :         200(2 ) ---------->    13:         200(2 ) <----------  
# 10:         200(2 ) ---------->      
# 11:         ACK(2 ) <*---------    14:         ACK(2 ) ---------->  
#                                    15:       Pause     [   1000ms]  
# 12:         BYE(2 ) <----------    16:         BYE(2 ) ---------->  
# 13:         200(2 ) ---------->    17:         200(2 ) <----------  
  def test_unexpected_message_during_pause_2calls
    test = SippTest.new("test_unexpected_message_during_pause_2calls", "-sf myuac_dialog_1secPause_2calls.sipp -mc -ar", "-sf myuas_dialog_double200_2calls.sipp  -mc ")
    #test.expected_exitstatus = 1
    #test.expected_error_log = /Aborting call on unexpected message for.*while pausing \(index 6\). Response \'200\' does not match.*Message index   7,\s*BYE\(1\)/m
    assert(test.run())
  end 
  
  
 # Since we use cumulative calls compared to received count to determine if msg has been pre-received
 # can we confuse sipp by having two simultaneous calls
#          SERVER                          CLIENT  
#                                    9 :      INVITE(2 ) ---------->  
# 0 :      INVITE(1 ) <----------    0 :      INVITE(1 ) ---------->    creates 2 cumaltive calls - will this allow double recd msgs?
#                                    1 :         100(1 ) <*---------  
# 1 :         180(1 ) ---------->    2 :         180(1 ) <*---------  
#                                    3 :         183(1 ) <*---------  
# 2 :         200(1 ) ---------->    4 :         200(1 ) <----------  
# 3 :         200(1 ) ---------->                                       should fail here
# 4 :         ACK(1 ) <*---------    5 :         ACK(1 ) ---------->  
#                                    6 :       Pause     [   1000ms]  
# 5 :         BYE(1 ) <----------    7 :         BYE(1 ) ---------->  
# 6 :         200(1 ) ---------->    8 :         200(1 ) <----------  
      
# 7 :      INVITE(2 ) <----------    
#                                    10:         100(2 ) <*---------  
# 8 :         180(2 ) ---------->    11:         180(2 ) <*---------  
#                                    12:         183(2 ) <*---------  
# 9 :         200(2 ) ---------->    13:         200(2 ) <----------  
# 10:         200(2 ) ---------->      
# 11:         ACK(2 ) <*---------    14:         ACK(2 ) ---------->  
#                                    15:       Pause     [   1000ms]  
# 12:         BYE(2 ) <----------    16:         BYE(2 ) ---------->  
# 13:         200(2 ) ---------->    17:         200(2 ) <----------  

  def test_unexpected_message_during_pause_2calls_earlyReg_call2
    test = SippTest.new("test_unexpected_message_during_pause_2calls_earlyReg_call2", "-sf myuac_dialog_1secPause_2calls_early2ndinvite.sipp -mc -ar", "-sf myuas_dialog_double200_2calls.sipp  -mc ")
    #test.expected_exitstatus = 1
    #test.expected_error_log = /Aborting call on unexpected message for.*while pausing \(index 7\). Response \'200\' does not match.*Message index   8,\s*BYE\(1\).*Message index  13, 200\(2\)/m
    assert(test.run())
  end 
  
# -yr option turns on outgoing retransmits for the client 
# -ar option turns on incoming absorb retransmits for server
# client will retransmit messages to server several times as we induce slow response with pauses.
  def test_slow_server_rxmt_client
    test = SippTest.new("test_slow_server_rxmt_client", "-sf  rxmt_client.sipp -mc -yr", "-sf slow_server.sipp  -mc -ar ")
    #test.expected_exitstatus = 1
    #test.expected_error_log = /Aborting call on unexpected message for.*while pausing \(index 7\). Response \'200\' does not match.*Message index   8,\s*BYE\(1\).*Message index  13, 200\(2\)/m
    assert(test.run())
  end 

# -yr used at both client and server to retransmit messages while other side is slow to respond
# -ar at both sides to absorb the retransmits from the other side.  
  def test_rxmt_server_slow_client
    test = SippTest.new("test_rxmt_server_slow_client", "-sf   slow_client.sipp -mc -yr -ar", "-sf rxmt_server.sipp -mc -yr ")
    #test.expected_exitstatus = 1
    #test.expected_error_log = /Aborting call on unexpected message for.*while pausing \(index 7\). Response \'200\' does not match.*Message index   8,\s*BYE\(1\).*Message index  13, 200\(2\)/m
    assert(test.run())
  end 

  
  
  
  #try delaying messages to the client as well
  #look for jeff's scenario
  #test combination of -yr -ar
  
  
  # def test_one_rxmt
    # test = SippTest.new("test_one_rxmt", "-sf bla_register_and_subscribe_two_lines_short_with_optional_client_opt-send.sipp -mc", "-sf bla_register_and_subscribe_two_lines_short_register_and_200_swapped_server_opt-send.sipp -mc")
    # test.expected_exitstatus = 255
    # test.expected_error_log = /Optional message 5\(4\) followed by message 7, a SEND message which is not allowed/
    # assert(test.run())
  # end    


end
