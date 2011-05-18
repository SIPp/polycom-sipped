#!/usr/bin/ruby
#
# Copyright (c) 2010, Polycom, Inc.
#
# Author: Edward Estabrook <edward.estabrook@polycom.com>
#


require 'test/unit'
require 'sipp_test'

class DumpSequenceDiagram < Test::Unit::TestCase

  def test_dump_sequence_diagram_mc
    test = SippTest.new("dump_sequence_diagram_mc", "-dump_sequence_diagram -sf dump_sequence_diagram1.sipp -mc -skip_rlimit")
	test.expected_client_output = %Q!    REGISTER(1 ) <---------- \r\n         200(1 ) ----------> \r\n   SUBSCRIBE(2 ) ----------> \r\n         200(2 ) <---------- \r\n      NOTIFY(2 ) <---------- \r\n         200(2 ) ----------> \r\n   SUBSCRIBE(3 ) <---------- \r\n         202(3 ) ----------> \r\n      NOTIFY(3 ) ----------> \r\n         200(3 ) <---------- \r\n\r\n    REGISTER(1 ) <---------- \r\n         200(1 ) ----------> \r\n     [ NOP ]                       \r\n      NOTIFY(2 ) <---------- \r\n         200(2 ) ----------> \r\n     [ NOP ]                       \r\n      INVITE(4 ) <---------- \r\n\r\n         183(4 ) ----------> \r\n      NOTIFY(2 ) <---------- \r\n         200(2 ) ----------> \r\n         200(4 ) ----------> \r\n         ACK(4 ) <---------- \r\n      NOTIFY(2 ) <---------- \r\n         200(2 ) ----------> \r\n       Pause     [   2000ms]         \r\n     [ NOP ]                       \r\n      INVITE(4 ) <---------- \r\n      INVITE(4 ) <---------- \r\n      INVITE(4 ) <---------- \r\n      INVITE(4 ) <---------- \r\n      INVITE(4 ) <---------- \r\n      INVITE(4 ) <---------- \r\n      INVITE(4 ) <---------- \r\n\r\n    REGISTER(1 ) <---------- \r\n    REGISTER(1 ) <---------- \r\n    REGISTER(1 ) <---------- \r\n    REGISTER(1 ) <---------- \r\n    REGISTER(1 ) <---------- \r\n    REGISTER(1 ) <---------- \r\n    REGISTER(1 ) <---------- \r\n    REGISTER(1 ) <---------- \r\n    REGISTER(1 ) <---------- \r\n    REGISTER(1 ) <---------- \r\n    REGISTER(7 ) <---------- \r\n    REGISTER(7 ) <---------- \r\n    REGISTER(7 ) <---------- \r\n    REGISTER(11) <---------- \r\n         200(11) ----------> \r\n   SUBSCRIBE(12) ----------> \r\n         200(12) <---------- \r\n      NOTIFY(12) <---------- \r\n         200(12) ----------> \r\n   SUBSCRIBE(13) <---------- \r\n         202(13) ----------> \r\n      NOTIFY(13) ----------> \r\n         200(13) <---------- \r\n\r\n       Pause     [   1000ms]         \r\n\r\n     [ NOP ]                       \r\n      NOTIFY(12) <---------- \r\n         200(12) ----------> \r\n     [ NOP ]                       \r\n      INVITE(14) <---------- \r\n\r\n         183(14) ----------> \r\n      NOTIFY(12) <---------- \r\n         200(12) ----------> \r\n         200(14) ----------> \r\n         ACK(14) <---------- \r\n      NOTIFY(12) <---------- \r\n         200(12) ----------> \r\n    REGISTER(11) <---------- \r\n         200(11) ----------> \r\n    REGISTER(11) <---------- \r\n         200(11) ----------> \r\n!
    assert(test.run())
  end

  def test_dump_sequence_diagram_default_uac
    test = SippTest.new("dump_sequence_diagram_uac", "-dump_sequence_diagram -sn uac -skip_rlimit")
	test.expected_client_output = %Q!          INVITE ----------> \r\n             100 <---------- \r\n             180 <---------- \r\n             183 <---------- \r\n             200 <---------- \r\n             ACK ----------> \r\n       Pause     [      0ms]         \r\n             BYE ----------> \r\n             200 <---------- \r\n\r\n!
    assert(test.run())
  end

end
