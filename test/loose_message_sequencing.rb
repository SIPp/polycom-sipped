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

# 0 :    REGISTER(1 ) <----------    0 :    REGISTER(1 ) ---------->              
# 1 :         200(1 ) ---------->    1 :         200(1 ) <----------              
# 2 :   SUBSCRIBE(2 ) <----------    2 :   SUBSCRIBE(2 ) ---------->              
# 3 :      RANDOM(7 ) ---------->    3 :      RANDOM(7 ) <*---------              
# 4 :      RANDOM(7 ) ---------->                  
# 5 :      RANDOM(4 ) ---------->                                     Fails here as required Register for dialog 4 has not yet been received            
# 6 :    REGISTER(4 ) ---------->    4 :    REGISTER(4 ) <----------              
# 7 :      RANDOM(4 ) ---------->    5 :      RANDOM(4 ) <*---------              
# 8 :         200(2 ) ---------->    6 :         200(2 ) <----------              
# 9 :         200(4 ) <----------    7 :         200(4 ) ---------->              
                  
# 10:      NOTIFY(2 ) ---------->    8 :      NOTIFY(2 ) <----------              
# 11:         200(2 ) <----------    9 :         200(2 ) ---------->              
# 12:   SUBSCRIBE(3 ) ---------->    10:   SUBSCRIBE(3 ) <----------              
# 13:         202(3 ) <----------    11:         202(3 ) ---------->              
# 14:      NOTIFY(3 ) <----------    12:      NOTIFY(3 ) ---------->              
# 15:         200(3 ) ---------->    13:         200(3 ) <----------              
                  
# 16:      NOTIFY(5 ) ---------->    14:      NOTIFY(5 ) <----------              


# tests failure and verifies failure message points to correct list of acceptable message indexes
  def test_bla_register_and_subscribe_two_lines_short_register_and_200_swapped_with_optional_too_early_will_fail
    test = SippTest.new("test_bla_register_and_subscribe_two_lines_short_register_and_200_swapped_with_optional_too_early_will_fail", "-sf bla_register_and_subscribe_two_lines_short_with_optional_client.sipp -mc", "-sf bla_register_and_subscribe_two_lines_short_register_and_200_swapped_with_optional_too_early_server.sipp -mc")
    test.expected_exitstatus = 1
    test.expected_error_log = /Request \'RANDOM\' does not match any expected message.*Message index   6, 200\(2\).*Message index  10, SUBSCRIBE\(3\).*Message index   4, REGISTER\(4\).*Message index  14, NOTIFY\(5\)/m
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
  
  
# a base test case , this should pass  
  def test_bla_register_sub
    test = SippTest.new("test_bla_register_sub", "-sf test_bla_register_sub_client.sipp  -mc", "-sf test_bla_register_sub_server.sipp -mc")
    assert(test.run())
  
  end
# simplify base case by removing second phone  
  def test_bla_register_sub_no2nd
    test = SippTest.new("test_bla_register_sub_no2nd", "-sf test_bla_register_sub_client_no2nd.sipp  -mc", "-sf test_bla_register_sub_server_no2nd.sipp -mc")
    assert(test.run())
  end

# test out of sequence notify to verify failure  
#             SERVER                      CLIENT
# 0 :    REGISTER(1 ) <----------  0 :    REGISTER(1 ) ---------->
# 1 :         200(1 ) ---------->  1 :         200(1 ) <----------
# 2 :   SUBSCRIBE(2 ) <----------  2 :   SUBSCRIBE(2 ) ---------->
#                                  3 :         200(2 ) <----------
# 3 :      NOTIFY(2 ) ---------->  4 :      NOTIFY(2 ) <----------  # fails here
# 4 :         200(2 ) ---------->  
# 5 :         200(2 ) <----------  5 :         200(2 ) ---------->
# 6 :   SUBSCRIBE(3 ) ---------->  6 :   SUBSCRIBE(3 ) <----------
# 7 :         202(3 ) <----------  7 :         202(3 ) ---------->
# 8 :      NOTIFY(3 ) <----------  8 :      NOTIFY(3 ) ---------->
# 9 :         200(3 ) ---------->  9 :         200(3 ) <----------

  def test_bla_register_sub_no2nd_early_notify
    test = SippTest.new("test_bla_register_sub_no2nd_early_notify", "-sf test_bla_register_sub_client_no2nd.sipp  -mc", "-sf test_bla_register_sub_server_early_notify.sipp -mc")
    test.expected_exitstatus = 1
    test.expected_error_log = /unexpected message.*while expecting \'200\'\s\(index 3\)\.\sRequest\s\'NOTIFY\'\sdoes not match.*Message index   3, 200\(2\).*Message index   6, SUBSCRIBE\(3\)/m
    assert(test.run())
  end
  
# test out of sequence notify around optional to verify late optional will fail
#
#             SERVER                      CLIENT       
# 0 :    REGISTER(1 ) <----------  0 :    REGISTER(1 ) ---------->
# 1 :         200(1 ) ---------->  1 :         200(1 ) <----------
# 2 :   SUBSCRIBE(2 ) <----------  2 :   SUBSCRIBE(2 ) ---------->
#                                  3 :         200(2 ) <*---------
# 3 :      NOTIFY(2 ) ---------->  4 :      NOTIFY(2 ) <----------  
# 4 :         200(2 ) ---------->                                    // should fail here
# 5 :         200(2 ) <----------  5 :         200(2 ) ---------->
# 6 :   SUBSCRIBE(3 ) ---------->  6 :   SUBSCRIBE(3 ) <----------
# 7 :         202(3 ) <----------  7 :         202(3 ) ---------->
# 8 :      NOTIFY(3 ) <----------  8 :      NOTIFY(3 ) ---------->
# 9 :         200(3 ) ---------->  9 :         200(3 ) <----------

  def test_bla_register_sub_no2nd_early_notify_optional_200
    test = SippTest.new("test_bla_register_sub_no2nd_early_notify_optional_200", "-sf test_bla_register_sub_client_no2nd_optional200.sipp -mc", "-sf test_bla_register_sub_server_early_notify.sipp -mc")
    test.expected_exitstatus = 1
    test.expected_error_log = /unexpected message.*while expecting \'SUBSCRIBE\' \(index 6\)\. Response \'200\' does not match any expected message.*Message index   6, SUBSCRIBE\(3\)/m
    assert(test.run())
  end

# test multiple optional recieves between sends - should fail parsing
#       
#             SERVER                      CLIENT
# 0 :    REGISTER(1 ) <----------  0 :    REGISTER(1 ) ---------->
# 1 :         200(1 ) ---------->  1 :         200(1 ) <----------
# 2 :   SUBSCRIBE(2 ) <----------  2 :   SUBSCRIBE(2 ) ---------->
#                                  3 :         200(2 ) <*---------
# 3 :      NOTIFY(2 ) ---------->  4 :      NOTIFY(2 ) <*---------  
# 4 :         200(2 ) ---------->                                    
# 5 :         200(2 ) <----------  5 :         200(2 ) ---------->
# 6 :   SUBSCRIBE(3 ) ---------->  6 :   SUBSCRIBE(3 ) <----------
# 7 :         202(3 ) <----------  7 :         202(3 ) ---------->
# 8 :      NOTIFY(3 ) <----------  8 :      NOTIFY(3 ) ---------->
# 9 :         200(3 ) ---------->  9 :         200(3 ) <----------
# exercises scenario::checkOptionalRecv
  def test_bla_register_sub_no2nd_early_notify_optional_200andNotify
    test = SippTest.new("test_bla_register_sub_no2nd_early_notify_optional_200andNotify", "-sf test_bla_register_sub_client_no2nd_optional200Notify.sipp -mc", "-sf test_bla_register_sub_server_early_notify.sipp -mc")
    test.expected_exitstatus = 1
    test.expected_error_log = /<recv> before <send> sequence without a mandatory message/m
    assert(test.run())
  end
  
  
  
# test waiting on optional, recv mandatory  - should pass
#       SERVER                              CLIENT
# 0 :    REGISTER(1 ) <----------    0 :    REGISTER(1 ) ---------->  
# 1 :         200(1 ) ---------->    1 :         200(1 ) <----------  
# 2 :   SUBSCRIBE(2 ) <----------    2 :   SUBSCRIBE(2 ) ---------->  
# 3 :         ACK(2 ) ---------->    3 :         ACK(2 ) <*---------  waiting on opt 
#                                    4 :         200(2 ) <*---------  
# 4 :      NOTIFY(2 ) ---------->    5 :      NOTIFY(2 ) <----------  recv mandatory
# 6 :         200(2 ) <----------    6 :         200(2 ) ---------->  
# 7 :   SUBSCRIBE(3 ) ---------->    7 :   SUBSCRIBE(3 ) <----------  
# 8 :         202(3 ) <----------    8 :         202(3 ) ---------->  
# 9 :      NOTIFY(3 ) <----------    9 :      NOTIFY(3 ) ---------->  
# 10:         200(3 ) ---------->    10:         200(3 ) <----------  


  def test_waiting_on_opt_recv_mand_same_dialog
    test = SippTest.new("test_waiting_on_opt_recv_mand_same_dialog", "-sf test_bla_register_sub_client_no2nd_optional200_wAck.sipp -mc", "-sf test_bla_register_sub_server_early_notify_wAck.sipp -mc")
    #test.expected_exitstatus = 1
    #test.expected_error_log = /<recv> before <send> sequence without a mandatory message/m
    assert(test.run())
  end
  
  

# early mandatory, other dialog while waiting on optional - should pass
#         SERVER                            CLIENT
# 0 :    REGISTER(1 ) <----------    0 :    REGISTER(1 ) ---------->  
# 1 :         200(1 ) ---------->    1 :         200(1 ) <----------  
# 2 :   SUBSCRIBE(2 ) <----------    2 :   SUBSCRIBE(2 ) ---------->  
# 3 :         ACK(2 ) ---------->    3 :         ACK(2 ) <*---------  waiting on opt 
# 7 :   SUBSCRIBE(3 ) ---------->                                    early subscribe
#                                    4 :         200(2 ) <*---------  
# 4 :      NOTIFY(2 ) ---------->    5 :      NOTIFY(2 ) <----------  recv mand
      
# 6 :         200(2 ) <----------    6 :         200(2 ) ---------->  
# 7 :   SUBSCRIBE(3 ) <----------  
# 8 :         202(3 ) <----------    8 :         202(3 ) ---------->  
# 9 :      NOTIFY(3 ) <----------    9 :      NOTIFY(3 ) ---------->  
# 10:         200(3 ) ---------->    10:         200(3 ) <----------  
  def test_waiting_on_opt_recv_mand_other_dialog
    test = SippTest.new("test_waiting_on_opt_recv_mand_other_dialog", "-sf test_bla_register_sub_client_no2nd_optional200_wAck.sipp -mc", "-sf test_bla_register_sub_server_early_notify_wAck_earlySubscribe.sipp -mc")
    #test.expected_exitstatus = 1
    #test.expected_error_log = /<recv> before <send> sequence without a mandatory message/m
    assert(test.run())
  end
  
# early out of sequence mandatory fr other dialog while waiting on optional - should fail
#         SERVER                            CLIENT  
# 0 :    REGISTER(1 ) <----------    0 :    REGISTER(1 ) ---------->  
# 1 :         200(1 ) ---------->    1 :         200(1 ) <----------  
# 2 :   SUBSCRIBE(3 ) ---------->    2 :   SUBSCRIBE(3 ) <----------  
# 3 :         202(3 ) <----------    3 :         202(3 ) ---------->  
# 4 :   SUBSCRIBE(2 ) <----------    4 :   SUBSCRIBE(2 ) ---------->  
#                                    5 :         ACK(2 ) <*---------  wait on optional
# 5 :         200(3 ) ---------->                                    out of seq mandatory fr other dialog
#                                    6 :         200(2 ) <*---------  
# 6 :      NOTIFY(2 ) ---------->    7 :      NOTIFY(2 ) <----------  
# 7 :         200(2 ) <----------    8 :         200(2 ) ---------->  
# 8 :      NOTIFY(3 ) <----------    9 :      NOTIFY(3 ) ---------->  
#                                    10:         200(3 ) <----------  

  def test_waiting_on_opt_recv_out_of_order_mand_other_dialog
    test = SippTest.new("test_waiting_on_opt_recv_out_of_order_mand_other_dialog", "-sf test_bla_register_sub_client_no2nd_optional200_wAck_earlyDiialog3.sipp -mc", "-sf test_bla_register_sub_server_early_notify_wAck_earlySubscribe_early200.sipp  -mc")
    test.expected_exitstatus = 1
    test.expected_error_log = /Aborting call on unexpected message for.*while expecting \'ACK\' \(index 5\). Response \'200\' does not match.*Message index   7.*Message index   9/m
    assert(test.run())
  end

#verify optional messages cannot be received if required mandatory messages for that dialog have not been processed while waiting on optional
#         SERVER                            CLIENT  
# 0 :    REGISTER(1 ) <----------    0 :    REGISTER(1 ) ---------->  
# 1 :         200(1 ) ---------->    1 :         200(1 ) <----------  
# 2 :   SUBSCRIBE(2 ) <----------    2 :   SUBSCRIBE(2 ) ---------->  
# 3 :   SUBSCRIBE(3 ) ---------->    3 :   SUBSCRIBE(3 ) <----------  
# 4 :         202(3 ) <----------    4 :         202(3 ) ---------->  
#                                    5 :         200(3 ) <*---------  Waiting optional dialog 3
# 5 :         ACK(2 ) ---------->                                    recv illegal early opt dialog 2
# 6 :         200(3 ) ---------->      
# 7 :      NOTIFY(2 ) ---------->    6 :      NOTIFY(2 ) <----------  
#                                    7 :         ACK(2 ) <*---------  
# 8 :         200(2 ) <----------    8 :         200(2 ) <----------  
#                                    9 :         200(2 ) ---------->  
# 9 :      NOTIFY(3 ) <----------    10:      NOTIFY(3 ) ---------->  
#                                    11:         200(3 ) <----------  

  def test_wait_opt_recv_opt_other_dialog_too_early
    test = SippTest.new("test_wait_opt_recv_opt_other_dialog_too_early", "-sf test_wait_opt_recv_opt_other_dialog_too_early_client.sipp -mc", "-sf test_wait_opt_recv_opt_other_dialog_too_early_server.sipp  -mc")
    test.expected_exitstatus = 1
    test.expected_error_log = /Aborting call on unexpected message for.*while expecting \'200\' \(index 5\). Request \'ACK\' does not match.*Message index   6.*Message index  10/m
    assert(test.run())
  end
  
#verify optional messages cannot be received if required mandatory messages for that dialog have not been processed while waiting on mandatory
#         SERVER                            CLIENT  
# 0 :    REGISTER(1 ) <----------    0 :    REGISTER(1 ) ---------->  
# 1 :         200(1 ) ---------->    1 :         200(1 ) <----------  
# 2 :   SUBSCRIBE(2 ) <----------    2 :   SUBSCRIBE(2 ) ---------->  
# 3 :   SUBSCRIBE(3 ) ---------->    3 :   SUBSCRIBE(3 ) <----------  
# 4 :         202(3 ) <----------    4 :         202(3 ) ---------->  
#                                    5 :         200(3 ) <----------  Waiting mandatory dialog 3
# 5 :         ACK(2 ) ---------->                                    recv illegal early opt dialog 2
# 6 :         200(3 ) ---------->      
# 7 :      NOTIFY(2 ) ---------->    6 :      NOTIFY(2 ) <----------  
#                                    7 :         ACK(2 ) <*---------  
# 8 :         200(2 ) <----------    8 :         200(2 ) <----------  
#                                    9 :         200(2 ) ---------->  
# 9 :      NOTIFY(3 ) <----------    10:      NOTIFY(3 ) ---------->  
#                                    11:         200(3 ) <----------  

  def test_wait_man_recv_opt_other_dialog_too_early
    test = SippTest.new("test_wait_man_recv_opt_other_dialog_too_early", "-sf test_wait_man_recv_opt_other_dialog_too_early_client.sipp -mc", "-sf test_wait_opt_recv_opt_other_dialog_too_early_server.sipp  -mc")
    test.expected_exitstatus = 1
    test.expected_error_log = /Aborting call on unexpected message for.*while expecting \'200\' \(index 5\). Request \'ACK\' does not match.*Message index   6.*Message index   5/m
    assert(test.run())
  end

#verify mandatory messages cannot be received if required mandatory messages for that dialog have not been processed while waiting on mandatory
#         SERVER                            CLIENT  
# 0 :    REGISTER(1 ) <----------    0 :    REGISTER(1 ) ---------->  
# 1 :         200(1 ) ---------->    1 :         200(1 ) <----------  
# 2 :   SUBSCRIBE(2 ) <----------    2 :   SUBSCRIBE(2 ) ---------->  
# 3 :   SUBSCRIBE(3 ) ---------->    3 :   SUBSCRIBE(3 ) <----------  
# 4 :         202(3 ) <----------    4 :         202(3 ) ---------->  
#                                    5 :         200(3 ) <----------  Waiting mandatory dialog 3
# 5 :         ACK(2 ) ---------->                                    recv illegal early mandatory dialog 2
# 6 :         200(3 ) ---------->      
# 7 :      NOTIFY(2 ) ---------->    6 :      NOTIFY(2 ) <----------  
#                                    7 :         ACK(2 ) <----------  
# 8 :         200(2 ) <----------    8 :         200(2 ) <----------  
#                                    9 :         200(2 ) ---------->  
# 9 :      NOTIFY(3 ) <----------    10:      NOTIFY(3 ) ---------->  
#                                    11:         200(3 ) <----------  

  def test_wait_man_recv_man_other_dialog_too_early
    test = SippTest.new("test_wait_man_recv_man_other_dialog_too_early", "-sf test_wait_man_recv_man_other_dialog_too_early_client.sipp -mc", "-sf test_wait_opt_recv_opt_other_dialog_too_early_server.sipp  -mc")
    test.expected_exitstatus = 1
    test.expected_error_log = /Aborting call on unexpected message for.*while expecting \'200\' \(index 5\). Request \'ACK\' does not match.*Message index   6.*Message index   5/m
    assert(test.run())
  end
 
# ref base - everything in one dialog using mc 
# SERVER    CLIENT
# 0 :    REGISTER(1 ) <----------    0 :    REGISTER(1 ) ---------->
# 1 :         200(1 ) ---------->    1 :         200(1 ) <----------
# 2 :   SUBSCRIBE(1 ) <----------    2 :   SUBSCRIBE(1 ) ---------->
# 3 :    REGISTER(1 ) ---------->    3 :    REGISTER(1 ) <----------
# 4 :         200(1 ) ---------->    4 :         200(1 ) <----------
# 5 :      NOTIFY(1 ) ---------->    5 :      NOTIFY(1 ) <----------
# 6 :         200(1 ) <----------    6 :         200(1 ) ---------->
# 7 :   SUBSCRIBE(1 ) ---------->    7 :   SUBSCRIBE(1 ) <----------
# 8 :         202(1 ) <----------    8 :         202(1 ) ---------->
# 9 :      NOTIFY(1 ) <----------    9 :      NOTIFY(1 ) ---------->
# 10:         200(1 ) ---------->    10:         200(1 ) <----------
# 11:      NOTIFY(1 ) ---------->    11:      NOTIFY(1 ) <----------

  def test_all_one_dialog_ref
    test = SippTest.new("test_all_one_dialog_ref", "-sf test_advancing_prereceived_after_send_client.sipp -mc", "-sf test_advancing_prereceived_after_send_server.sipp  -mc")
    #test.expected_exitstatus = 1
    #test.expected_error_log = /Aborting call on unexpected message for.*while expecting \'200\' \(index 5\). Request \'ACK\' does not match.*Message index   6.*Message index   5/m
    assert(test.run())
  end
#         SERVER                            CLIENT      
# 0 :    REGISTER(1 ) <----------    0 :    REGISTER(1 ) ---------->  
# 3 :    REGISTER(2 ) ---------->                                     early legal receipt
# 1 :         200(1 ) ---------->    1 :         200(1 ) <----------  
# 2 :   SUBSCRIBE(1 ) <----------    2 :   SUBSCRIBE(1 ) ---------->   this send should trigger jumping over prereceived
#                                    11:      NOTIFY(1 ) <*----------  waiting on optional should not occur - prereceived next message
#                                    3 :    REGISTER(2 ) <---------  
# 4 :         200(1 ) ---------->    4 :         200(1 ) <----------  
# 5 :      NOTIFY(1 ) ---------->    5 :      NOTIFY(1 ) <----------  
# 6 :         200(1 ) <----------    6 :         200(1 ) ---------->  
# 7 :   SUBSCRIBE(1 ) ---------->    7 :   SUBSCRIBE(1 ) <----------  
# 8 :         202(1 ) <----------    8 :         202(1 ) ---------->  
# 9 :      NOTIFY(1 ) <----------    9 :      NOTIFY(1 ) ---------->  
# 10:         200(1 ) ---------->    10:         200(1 ) <----------  
      
  def test_skipping_prereceived_after_send
    test = SippTest.new("test_skipping_prereceived_after_send", "-sf test_advancing_prereceived_after_send_client_opt_notify.sipp -mc", "-sf test_advancing_prereceived_after_send_server_early_reg.sipp  -mc")
    assert(test.run())
  end
  
  
  
  #         SERVER                            CLIENT      
# 0 :    REGISTER(1 ) <----------    0 :    REGISTER(1 ) ---------->  
# 3 :    REGISTER(2 ) ---------->                                     early legal receipt
# 1 :         200(1 ) ---------->    1 :         200(1 ) <----------  
# 2 :   SUBSCRIBE(1 ) <----------    2 :   SUBSCRIBE(1 ) ---------->   this send should trigger jumping over prereceived
#                                    11:      NOTIFY(1 ) <*----------  waiting on optional should not occur - prereceived next message
# 3 :    REGISTER(2 ) ---------->    3 :    REGISTER(2 ) <---------   double register should fail  
# 4 :         200(1 ) ---------->    4 :         200(1 ) <----------  
# 5 :      NOTIFY(1 ) ---------->    5 :      NOTIFY(1 ) <----------  
# 6 :         200(1 ) <----------    6 :         200(1 ) ---------->  
# 7 :   SUBSCRIBE(1 ) ---------->    7 :   SUBSCRIBE(1 ) <----------  
# 8 :         202(1 ) <----------    8 :         202(1 ) ---------->  
# 9 :      NOTIFY(1 ) <----------    9 :      NOTIFY(1 ) ---------->  
# 10:         200(1 ) ---------->    10:         200(1 ) <----------  
      
  def test_skipping_prereceived_after_send_w_illegal_double_reg
    test = SippTest.new("test_skipping_prereceived_after_send_w_illegal_double_reg", "-sf test_advancing_prereceived_after_send_client_opt_notify.sipp -mc", "-sf test_advancing_prereceived_after_send_server_early_reg_and_twice.sipp  -mc")
    test.expected_exitstatus = 1
    test.expected_error_log = /Aborting call on unexpected message for.*while expecting \'200\' \(index 5\). Request \'REGISTER\' does not match.*Message index   5,\s*200\(1\)/m
    assert(test.run())
  end
  
# test multiple calls (invites?) both initiated and received since this is our 
# reference for wether or not messages are already received.  

# another reference cases before we continue exercising loose_message_sequence  
 #              sERVER                            CLIENT
# 0 :          INVITE <----------    0 :          INVITE ---------->
#                                    1 :             100 <*---------
# 1 :             180 ---------->    2 :             180 <*---------
#                                    3 :             183 <*---------
# 2 :             200 ---------->    4 :             200 <----------
# 3 :             ACK <*---------    5 :             ACK ---------->
#                                    6 :       Pause     [      0ms]
# 4 :             BYE <----------    7 :             BYE ---------->
# 5 :             200 ---------->    8 :             200 <----------
 
   def test_uas_uac
    test = SippTest.new("test_uas_uac", "-sf myuac.sipp -mc -m 10", "-sf myuas.sipp  -mc -m 10")
    #test.expected_exitstatus = 1
    #test.expected_error_log = /Aborting call on unexpected message for.*while expecting \'200\' \(index 5\). Request \'REGISTER\' does not match.*Message index   5,\s*200\(1\)/m
    assert(test.run())
  end 
  
   #              sERVER                            CLIENT
# 0 :      INVITE(1 ) <----------    0 :      INVITE(1 ) ---------->
#                                    1 :         100(1 ) <*---------
# 1 :         180(1 ) ---------->    2 :         180(1 ) <*---------
#                                    3 :         183(1 ) <*---------
# 2 :         200(1 ) ---------->    4 :         200(1 ) <----------
# 3 :         ACK(1 ) <*---------    5 :         ACK(1 ) ---------->
#                                    6 :       Pause     [      0ms]
# 4 :         BYE(1 ) <----------    7 :         BYE(1 ) ---------->
# 5 :         200(1 ) ---------->    8 :         200(1 ) <----------

 
   def test_uas_uac_dialog
    test = SippTest.new("test_uas_uac_dialog", "-sf myuac_dialog.sipp -mc -m 10", "-sf myuas_dialog.sipp  -mc -m 10")
    #test.expected_exitstatus = 1
    #test.expected_error_log = /Aborting call on unexpected message for.*while expecting \'200\' \(index 5\). Request \'REGISTER\' does not match.*Message index   5,\s*200\(1\)/m
    assert(test.run())
  end 
  
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
    test = SippTest.new("test_unexpected_message_during_pause", "-sf myuac_dialog_1secPause.sipp -mc ", "-sf myuas_dialog_double200.sipp  -mc ")
    test.expected_exitstatus = 1
    test.expected_error_log = /Aborting call on unexpected message for.*while pausing \(index 6\). Response \'200\' does not match.*Message index   7,\s*BYE\(1\)/m
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
    test = SippTest.new("test_unexpected_message_during_pause_2calls", "-sf myuac_dialog_1secPause_2calls.sipp -mc ", "-sf myuas_dialog_double200_2calls.sipp  -mc ")
    test.expected_exitstatus = 1
    test.expected_error_log = /Aborting call on unexpected message for.*while pausing \(index 6\). Response \'200\' does not match.*Message index   7,\s*BYE\(1\)/m
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
    test = SippTest.new("test_unexpected_message_during_pause_2calls_earlyReg_call2", "-sf myuac_dialog_1secPause_2calls_early2ndinvite.sipp -mc ", "-sf myuas_dialog_double200_2calls.sipp  -mc ")
    test.expected_exitstatus = 1
    test.expected_error_log = /Aborting call on unexpected message for.*while pausing \(index 7\). Response \'200\' does not match.*Message index   8,\s*BYE\(1\).*Message index  13, 200\(2\)/m
    assert(test.run())
  end 
  
  
  
#           SERVER                            CLIENT  
# 0 :      INVITE(1 ) <----------    0 :      INVITE(1 ) ---------->  
# 1 :      INVITE(2 ) <----------    9 :      INVITE(2 ) ---------->  
#                                    1 :         100(1 ) <*---------  
# 4 :         180(1 ) ---------->    2 :         180(1 ) <*---------  
#                                    3 :         183(1 ) <*---------  
      
# 2 :         200(2 ) ---------->                                        early 200 legal
# 5 :         200(1 ) ---------->    4 :         200(1 ) <----------  
# 3 :         200(2 ) ---------->                                        2nd copy of early 200 - illegal, should fail
# 6 :         ACK(1 ) <*---------    5 :         ACK(1 ) ---------->  
#                                    6 :       Pause     [   1000ms]      should block here, no legal incoming messages at this point
# 7 :         BYE(1 ) <----------    7 :         BYE(1 ) ---------->        both dialogs require a sendmessage next
# 8 :         200(1 ) ---------->    8 :         200(1 ) <----------  
      
      
#                                    10:         100(2 ) <*---------  
# 9 :         180(2 ) ---------->    11:         180(2 ) <*---------  
#                                    12:         183(2 ) <*---------  
# 10:         200(2 ) ---------->    13:         200(2 ) <----------  
# 11:         ACK(2 ) <*---------    14:         ACK(2 ) ---------->  
#                                    15:       Pause     [   1000ms]  
# 12:         BYE(2 ) <----------    16:         BYE(2 ) ---------->  
# 13:         200(2 ) ---------->    17:         200(2 ) <----------  
      


  def test_unexpected_duplicate_early_message_during_2calls
    test = SippTest.new("test_unexpected_duplicate_early_message_during_2calls", "-sf myuac_dialog_1secPause_2calls_ack.sipp -mc ", "-sf myuas_dialog_double200_2calls_earlydouble200.sipp  -mc ")
    test.expected_exitstatus = 1
    test.expected_error_log = /Aborting call on unexpected message for.*while pausing \(index 7\). Response \'200\' does not match.*Message index   8,\s*BYE\(1\).*Message index  14, ACK\(2\)/m
    assert(test.run())
  end 


# test multiple calls and detection of duplicate future mandatory messages  
# 0 :      INVITE(1 ) <----------    0 :      INVITE(1 ) ---------->  
#                                    1 :         100(1 ) <*---------  
# 1 :         180(1 ) ---------->    2 :         180(1 ) <*---------  
#                                    3 :         183(1 ) <*---------  
# 2 :         200(1 ) ---------->    4 :         200(1 ) <----------  
# 3 :         ACK(1 ) <*---------    5 :         ACK(1 ) ---------->  
#                                    6 :       Pause     [   1000ms]  
# 4 :         BYE(1 ) <----------    7 :         BYE(1 ) ---------->  
# 5 :         200(1 ) ---------->    8 :         200(1 ) <----------  
      
# 6 :      INVITE(2 ) <----------    9 :      INVITE(2 ) ---------->  
#                                    10:         100(2 ) <*---------  
# 7 :         180(2 ) ---------->    11:         180(2 ) <*---------  
#                                    12:         183(2 ) <*---------  
# 8 :         200(2 ) ---------->    13:         200(2 ) <----------  
# 9 :         ACK(2 ) <*---------    14:         ACK(2 ) ---------->  
#                                    15:       Pause     [   1000ms]  
# 10:         BYE(2 ) <----------    16:         BYE(2 ) ---------->  
# 11:         200(2 ) ---------->    17:         200(2 ) <----------  2 calls complete
      
# 12:   SUBSCRIBE(3 ) <----------    18:   SUBSCRIBE(3 ) ---------->  
# 13:      NOTIFY(3 ) ---------->                                        early notify -legal
# 14:      NOTIFY(3 ) ---------->                                        duplicate - should fail
# 15:         200(3 ) ---------->    19:         200(3 ) <----------  
#                                    20:      NOTIFY(3 ) <----------  
# 16:         200(3 ) <----------    21:         200(3 ) ---------->  

  def test_multiple_calls
    test = SippTest.new("test_multiple_calls", "-sf myuac_dialog_2calls.sipp -mc ", "-sf myuas_dialog_2calls.sipp  -mc ")
    test.expected_exitstatus = 1
    test.expected_error_log = /Aborting call on unexpected message for.*while expecting \'200\' \(index 19\). Request \'NOTIFY\' does not match.*Message index\s*19,\s*200\(3\)/m
    assert(test.run())
  end 
 
# received messages are handled by call::process_incoming which has special advancing logic to determine next message
# pause nop send  messages utilize call::next()   which has seperate call to determine next message for loose_message_sequence

# 0 :   SUBSCRIBE(3 ) <----------    0 :   SUBSCRIBE(3 ) ---------->
# 1 :      NOTIFY(3 ) ---------->                                       legal early message, early recv across pause and nops ok
# 2 :         200(3 ) ---------->    1 :         200(3 ) <*---------
# 3 :   SUBSCRIBE(4 ) ---------->    2 :   SUBSCRIBE(4 ) <----------
#                                    3 :       Pause     [    500ms]
#                                    4 : NOP::Richard was Here          should be advancing beyond prerecieved Notify to complete
#                                    5 :      NOTIFY(3 ) <----------
# 4 :         200(3 ) <----------    6 :         200(3 ) ---------->
  
  def test_early_man_across_nop_pause
    test = SippTest.new("test_early_man_across_nop_pause", "-sf test_early_mandatory_across_noppause_client.sipp -mc ", "-sf test_early_mandatory_across_noppause_server.sipp  -mc ")
    assert(test.run())
  end   
  
# 0 :   SUBSCRIBE(3 ) <----------    0 :   SUBSCRIBE(3 ) ---------->    
# 1 :      NOTIFY(3 ) ---------->      sent early  
# 2 :         200(3 ) ---------->    1 :         200(3 ) <*---------    
# 3 :   SUBSCRIBE(4 ) ---------->    2 :   SUBSCRIBE(4 ) <----------    
#                                    3 :       Pause     [    500ms]    
#                                    4 : NOP::Richard was Here    
# 4 :       NOTIFY(4) <----------     5 :      NOTIFY(4)  ----------->  send should not be skipped over during advancing to prereceived  
#                                    6 :      NOTIFY(3 ) <----------  pre-received should be processed by send advancing logic  
# 4 :         200(3 ) <----------     7 :      200(3 )    ---------->    
        


  def test_early_man_across_nop_pause_followed_by_send
    test = SippTest.new("test_early_man_across_nop_pause_followed_by_send", "-sf test_early_mandatory_across_noppause_followedbyoutgoing_client.sipp -mc ", "-sf test_early_mandatory_across_noppause_followedbyoutgoing_server.sipp  -mc ")
    assert(test.run())
  end   
 
# 0 :   SUBSCRIBE(3 ) <----------    0 :   SUBSCRIBE(3 ) ---------->    
# 1 :      NOTIFY(3 ) ---------->                                        legal early notify   
#                                    1 :         200(3 ) <*---------    
# 2 :   SUBSCRIBE(4 ) ---------->    2 :   SUBSCRIBE(4 ) <----------    
# 3 :         200(3 ) ---------->                                         late opt 200: should fail  
#                                    3 :       Pause     [    500ms]    
#                                    4 : NOP::Richard was Here    
# 4 :      NOTIFY(4 ) <----------    5 :      NOTIFY(4 ) ---------->    
#                                    6 :      NOTIFY(3 ) <----------    
# 5 :         200(3 ) <----------    7 :         200(3 ) ---------->    

 
   def test_early_man_across_nop_pause_followed_by_send_late_200
    test = SippTest.new("test_early_man_across_nop_pause_followed_by_send_late_200", "-sf test_early_mandatory_across_noppause_followedbyoutgoing_client.sipp -mc ", "-sf test_early_mandatory_across_noppause_followedbyoutgoing_server_late_opt_200.sipp -mc ")
    test.expected_exitstatus = 1
    #test.expected_error_log = /Aborting call on unexpected message for.*while pausing \(index 3\). Response \'200\' does not match.*Message index\s*6,\s*NOTIFY\(3\).*Message index\s*5, NOTIFY\(4\)/m
    test.expected_error_log = /Aborting call on unexpected message for.*while pausing \(index 3\). Response \'200\' does not match.*Message index\s*7,\s*200\(3\).*Message index\s*5, NOTIFY\(4\)/m
    assert(test.run())
  end   
 
 # last message is optional : should fail
  def test_last_message_optional
    test = SippTest.new("test_last_message_optional", "-sf myuac_dialog_last_message_optional.sipp -mc ", "-sf myuas_dialog.sipp -mc ")
    test.expected_exitstatus = 255
    test.expected_error_log = / Last Message \(index = 8\) cannot be on optional message./
    assert(test.run())
  end   
  
  
  # multiple optional messages
  def test_multiple_optional_messages
    test = SippTest.new("test_multiple_optional_messages", "-sf test_multiple_optionals_client.sipp -mc ", "-sf test_multiple_optionals_server.sipp -mc ")
    assert(test.run())
  end   
  
  # send message should block future optionals from being received early
#         SERVER                            CLIENT  
# 0 :    REGISTER(1 ) <----------    0 :    REGISTER(1 ) ---------->        
# 1 :    REGISTER(2 ) <----------    1 :    REGISTER(2 ) ---------->        
# 2 :         200(1 ) ---------->    2 :         200(1 ) <*---------        
#                                    3 :       PRACK(1 ) <*---------        
#                                    4 :         100(1 ) <*---------        
# 3 :    REGISTER(1 ) ---------->    5 :    REGISTER(1 ) <*---------        
# 4 :         200(1 ) ---------->                                       late 200 legal      
# 5 :      NOTIFY(1 ) ---------->    6 :      NOTIFY(1 ) <*---------        
# 6 :         200(1 ) ---------->                                       dupl opt 200 legal      
# 7 :      NOTIFY(1 ) ---------->                                       dupl opt Notify legal      
# 8 :         200(1 ) ---------->                                       dupl opt 200 legal      
# 9 :         200(1 ) ---------->                                       dupl opt 200 legal      
# 10:      NOTIFY(1 ) ---------->                                       dupl opt Notify legal      
# 11:   SUBSCRIBE(1 ) ---------->                                       should fail      
# 12:         200(2 ) ---------->    7 :         200(2 ) <----------        
# 13:         200(1 ) <----------    8 :         200(1 ) ---------->    send blocks access to future subscribe      
# 14:   SUBSCRIBE(1 ) ---------->    9 :   SUBSCRIBE(1 ) <*---------        
# 15:         ACK(1 ) ---------->    10:         ACK(1 ) <----------        
# 16:         202(1 ) <----------    11:         202(1 ) ---------->        

  def test_sendmessage_block_rx_future_message
    test = SippTest.new("test_sendmessage_block_rx_future_message", "-sf test_send_should_block_future_optionals_client.sipp -mc ", "-sf test_send_should_block_future_optionals_server.sipp -mc ")
    test.expected_exitstatus = 255
    test.expected_error_log = /Aborting call on unexpected message for.*while expecting \'200\' \(index 2\)\. Request \'SUBSCRIBE\' does not match any expected message.*Message index   8, 200\(1\) - SendingMessage.*Message index   7, 200\(2\)/m
    assert(test.run())
  end   

  # Test mutliple early messages within SAME dialog, with same message occuring multiple times
  # verify advancing happens when pre-requisite message is sent
  # next after send vs next after recv have different logic paths.
  # this tests also proves send msg properly advances over pre-received messages 
#         SERVER                             CLIENT  
# 0 :    REGISTER(1 ) <----------    0 :    REGISTER(1 ) ---------->          
# 1 :    REGISTER(2 ) ---------->              
# 2 :      NOTIFY(2 ) ---------->              
# 3 :         ACK(2 ) ---------->              
# 4 :      NOTIFY(2 ) ---------->               
# 5 :         200(1 ) ---------->    1 :         200(1 ) <----------          
# 6 :   SUBSCRIBE(1 ) <----------    2 :   SUBSCRIBE(1 ) ---------->   send should trigger advancing over early recvd msgs       
#                                    3 :      NOTIFY(1 ) <*---------  non rec'd optional should not block advancing        
#                                    4 :    REGISTER(2 ) <----------  received early        
#                                    5 :      NOTIFY(2 ) <----------  received early        
#                                    6 :         ACK(2 ) <----------  received early        
#                                    7 :      NOTIFY(2 ) <----------  received early        
# 7 :         200(1 ) ---------->    8 :         200(1 ) <----------          
# 8 :      NOTIFY(1 ) ---------->    9 :      NOTIFY(1 ) <----------          
# 9 :         200(1 ) <----------    10:         200(1 ) ---------->          
# 10:   SUBSCRIBE(1 ) ---------->    11:   SUBSCRIBE(1 ) <----------          
# 11:         202(1 ) <----------    12:         202(1 ) ---------->          
# 12:      NOTIFY(1 ) <----------    13:      NOTIFY(1 ) ---------->          
# 13:         200(1 ) ---------->    14:         200(1 ) <----------          
                
  def test_recv_advances_multiple_early_messages
    test = SippTest.new("test_recv_advances_multiple_early_messages", "-sf test_recv_advances_multiple_early_messages_client.sipp -mc ", "-sf test_recv_advances_multiple_early_messages_server.sipp -mc ")
    assert(test.run())
  end   
 
# test multiple early messages DIFFERENT dialogs and verify advancing when prerequesite message is sent
#         SERVER                            CLIENT    
# 0 :    REGISTER(1 ) <----------    0 :    REGISTER(1 ) ---------->  
# 1 :    REGISTER(2 ) ---------->      
# 2 :      NOTIFY(3 ) ---------->      
# 3 :         ACK(4 ) ---------->      
# 4 :      NOTIFY(5 ) ---------->      
# 5 :         200(1 ) ---------->    1 :         200(1 ) <----------  
# 6 :   SUBSCRIBE(1 ) <----------    2 :   SUBSCRIBE(1 ) ---------->  
    #                                3 :      NOTIFY(1 ) <*---------  optional not rec'd
    #                                4 :    REGISTER(2 ) <----------  early 1
    #                                5 :      NOTIFY(3 ) <----------  early 2
    #                                6 :         ACK(4 ) <----------  early 3
    #                                7 :      NOTIFY(5 ) <----------  early 4
# 7 :         200(1 ) ---------->    8 :         200(1 ) <----------  
# 8 :      NOTIFY(1 ) ---------->    9 :      NOTIFY(1 ) <----------  
# 9 :         200(1 ) <----------    10:         200(1 ) ---------->  
# 10:   SUBSCRIBE(1 ) ---------->    11:   SUBSCRIBE(1 ) <----------  
# 11:         202(1 ) <----------    12:         202(1 ) ---------->  
# 12:      NOTIFY(1 ) <----------    13:      NOTIFY(1 ) ---------->  
# 13:         200(1 ) ---------->    14:         200(1 ) <----------  

  def test_recv_advances_multiple_early_messages_wide
    test = SippTest.new("test_recv_advances_multiple_early_messages_wide", "-sf test_recv_advances_multiple_early_messages_wide_client.sipp -mc ", "-sf test_recv_advances_multiple_early_messages_wide_server.sipp -mc ")
    assert(test.run())
  end

# order within early rec'd messages should be indep between dialogs  
#         SERVER                      CLIENT
# 0 :    REGISTER(1 ) <----------    0 :    REGISTER(1 ) ---------->
# 1 :    REGISTER(2 ) ---------->    
# 2 :      NOTIFY(5 ) ---------->    
# 3 :         ACK(4 ) ---------->    
# 4 :      NOTIFY(3 ) ---------->    
# 5 :         200(1 ) ---------->    1 :         200(1 ) <----------
# 6 :   SUBSCRIBE(1 ) <----------    2 :   SUBSCRIBE(1 ) ---------->
    #                                3 :      NOTIFY(1 ) <*---------
    #                                4 :    REGISTER(2 ) <----------
    #                                5 :      NOTIFY(3 ) <----------
    #                                6 :         ACK(4 ) <----------
    #                                7 :      NOTIFY(5 ) <----------
# 7 :         200(1 ) ---------->    8 :         200(1 ) <----------
# 8 :      NOTIFY(1 ) ---------->    9 :      NOTIFY(1 ) <----------
# 9 :         200(1 ) <----------    10:         200(1 ) ---------->
# 10:   SUBSCRIBE(1 ) ---------->    11:   SUBSCRIBE(1 ) <----------
# 11:         202(1 ) <----------    12:         202(1 ) ---------->
# 12:      NOTIFY(1 ) <----------    13:      NOTIFY(1 ) ---------->
# 13:         200(1 ) ---------->    14:         200(1 ) <----------


 def test_recv_advances_multiple_early_messages_wide_reverseDislogOrder
    test = SippTest.new("test_recv_advances_multiple_early_messages_wide_reverseDislogOrder", "-sf test_recv_advances_multiple_early_messages_wide_client.sipp -mc ", "-sf test_recv_advances_multiple_early_messages_wide_reversedialogorder_server.sipp -mc ")
    assert(test.run())
  end   

# next after send vs next after recv have different logic paths.
# this tests that recv msg properly advances over pre-received messages 
  #       SERVER                              CLIENT
# 0 :    REGISTER(1 ) <----------    0 :    REGISTER(1 ) ---------->    
# 1 :    REGISTER(2 ) ---------->        
# 2 :      NOTIFY(5 ) ---------->        
# 3 :         ACK(4 ) ---------->        
# 4 :      NOTIFY(3 ) ---------->        
# 5 :         200(1 ) ---------->    1 :         200(1 ) <----------  recv triggers catchup  
    #                                2 :      NOTIFY(1 ) <*---------    
    #                                3 :    REGISTER(2 ) <----------  early 1  
    #                                4 :      NOTIFY(3 ) <----------  early 4  
    #                                5 :         ACK(4 ) <----------  early 3  
    #                                6 :      NOTIFY(5 ) <----------  early 2  
# 7 :         200(1 ) ---------->    7 :         200(1 ) <----------    
# 8 :      NOTIFY(1 ) ---------->    8 :      NOTIFY(1 ) <----------    
# 9 :         200(1 ) <----------    9 :         200(1 ) ---------->    
# 10:   SUBSCRIBE(1 ) ---------->    10:   SUBSCRIBE(1 ) <----------    
# 11:         202(1 ) <----------    11:         202(1 ) ---------->    
# 12:      NOTIFY(1 ) <----------    12:      NOTIFY(1 ) ---------->    
# 13:         200(1 ) ---------->    13:         200(1 ) <----------    
        

 
  def test_recv_advances_multiple_early_messages_wide_catchupTriggeredByrecv
    test = SippTest.new("test_recv_advances_multiple_early_messages_wide_catchupTriggeredByrecv", "-sf test_recv_advances_multiple_early_messages_wide_client_catchupTriggeredByrecv.sipp -mc ", "-sf test_recv_advances_multiple_early_messages_wide_reversedialogorder_server_catchupTriggeredByrecv.sipp -mc ")
    assert(test.run())
  end   

  
  
  
end

