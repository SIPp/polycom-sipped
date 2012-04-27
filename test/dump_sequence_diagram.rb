#!/usr/bin/ruby
#
# Copyright (c) 2010, Polycom, Inc.
#
# Author: Edward Estabrook <edward.estabrook@polycom.com>
#


require 'test/unit'
require './sipp_test'

class DumpSequenceDiagram < Test::Unit::TestCase

  def test_dump_sequence_diagram_mc
    test = SippTest.new("dump_sequence_diagram_mc", "-dump_sequence_diagram -sf dump_sequence_diagram1.sipp -mc -skip_rlimit")
    test.expected_client_output = %Q!0 :    REGISTER(1 ) <---------- \r\n1 :         200(1 ) ----------> \r\n2 :   SUBSCRIBE(2 ) ----------> \r\n3 :         200(2 ) <---------- \r\n4 :      NOTIFY(2 ) <---------- \r\n5 :         200(2 ) ----------> \r\n6 :   SUBSCRIBE(3 ) <---------- \r\n7 :         202(3 ) ----------> \r\n8 :      NOTIFY(3 ) ----------> \r\n9 :         200(3 ) <---------- \r\n\r\n10:    REGISTER(1 ) <---------- \r\n11:         200(1 ) ----------> \r\n12:     [ NOP ]                       \r\n13:      NOTIFY(2 ) <---------- \r\n14:         200(2 ) ----------> \r\n15:     [ NOP ]                       \r\n16:      INVITE(4 ) <---------- \r\n\r\n17:         183(4 ) ----------> \r\n18:      NOTIFY(2 ) <---------- \r\n19:         200(2 ) ----------> \r\n20:         200(4 ) ----------> \r\n21:         ACK(4 ) <---------- \r\n22:      NOTIFY(2 ) <---------- \r\n23:         200(2 ) ----------> \r\n24:       Pause     [   2000ms]         \r\n25:     [ NOP ]                       \r\n26:      INVITE(4 ) <---------- \r\n27:      INVITE(4 ) <---------- \r\n28:      INVITE(4 ) <---------- \r\n29:      INVITE(4 ) <---------- \r\n30:      INVITE(4 ) <---------- \r\n31:      INVITE(4 ) <---------- \r\n32:      INVITE(4 ) <---------- \r\n\r\n33:    REGISTER(1 ) <---------- \r\n34:    REGISTER(1 ) <---------- \r\n35:    REGISTER(1 ) <---------- \r\n36:    REGISTER(1 ) <---------- \r\n37:    REGISTER(1 ) <---------- \r\n38:    REGISTER(1 ) <---------- \r\n39:    REGISTER(1 ) <---------- \r\n40:    REGISTER(1 ) <---------- \r\n41:    REGISTER(1 ) <---------- \r\n42:    REGISTER(1 ) <---------- \r\n43:    REGISTER(7 ) <---------- \r\n44:    REGISTER(7 ) <---------- \r\n45:    REGISTER(7 ) <---------- \r\n46:    REGISTER(11) <---------- \r\n47:         200(11) ----------> \r\n48:   SUBSCRIBE(12) ----------> \r\n49:         200(12) <---------- \r\n50:      NOTIFY(12) <---------- \r\n51:         200(12) ----------> \r\n52:   SUBSCRIBE(13) <---------- \r\n53:         202(13) ----------> \r\n54:      NOTIFY(13) ----------> \r\n55:         200(13) <---------- \r\n\r\n56:       Pause     [   1000ms]         \r\n\r\n57:     [ NOP ]                       \r\n58:      NOTIFY(12) <---------- \r\n59:         200(12) ----------> \r\n60:     [ NOP ]                       \r\n61:      INVITE(14) <---------- \r\n\r\n62:         183(14) ----------> \r\n63:      NOTIFY(12) <---------- \r\n64:         200(12) ----------> \r\n65:         200(14) ----------> \r\n66:         ACK(14) <---------- \r\n67:      NOTIFY(12) <---------- \r\n68:         200(12) ----------> \r\n69:    REGISTER(11) <---------- \r\n70:         200(11) ----------> \r\n71:    REGISTER(11) <---------- \r\n72:         200(11) ----------> \r\n!
    assert(test.run())
  end

  def test_dump_sequence_diagram_default_uac
    test = SippTest.new("dump_sequence_diagram_uac", "-dump_sequence_diagram -sn uac -skip_rlimit")
    test.expected_client_output = %Q!0 :          INVITE ----------> \r\n1 :             100 <*--------- \r\n2 :             180 <*--------- \r\n3 :             183 <*--------- \r\n4 :             200 <---------- \r\n5 :             ACK ----------> \r\n6 :       Pause     [      0ms]         \r\n7 :             BYE ----------> \r\n8 :             200 <---------- \r\n\r\n!
    assert(test.run())
  end

end
