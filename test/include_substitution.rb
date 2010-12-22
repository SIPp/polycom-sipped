#!/usr/bin/ruby
#
# Copyright (c) 2010, Polycom, Inc.
#
# Author: Edward Estabrook <edward.estabrook@polycom.com>
#


require 'test/unit'
require 'sipp_test'

class IncludeDirectory < Test::Unit::TestCase
  def test_include_directory
    test = SippTest.new("include_substitution", "-sf include_substitution.sipp -mc -dump_xml -skip_rlimit")
	test.expected_client_output = %Q!<?xml version="1.0" encoding="ISO-8859-1" ?>\n<\!DOCTYPE scenario SYSTEM "sipp.dtd">\n\n<scenario name="include_substitution" params="-mc">\n\n  <\!-- include file in current sub-directory, replacement is aA=>1, Bb=>4, cc=>2 -->\n  \n  <\!-- include_substitution_1 file has directly specified parameters -->\n  \n  <\!-- include_substitution_2 file in sub-directory, inherits substitution -->\n  \n  <send request="INVITE" dialog="04">\n   <\![CDATA[\n      REGISTER sip:[service]@[remote_ip]:[remote_port] SIP/2.0\n      Content-Length: [len]\n    ]]>\n  </send>\n\n  <send request="BYE" dialog="01">\n   <\![CDATA[\n      REGISTER sip:[service]@[remote_ip]:[remote_port] SIP/2.0\n      cseq: [cseq dialog="04"]\n      Content-Length: [len]\n    ]]>\n  </send>\n  \n\n\n\n\n  \n\n  <send request="INVITE" dialog="01">\n   <\![CDATA[\n      REGISTER sip:[service]@[remote_ip]:[remote_port] SIP/2.0\n      Content-Length: [len]\n    ]]>\n  </send>\n\n  <send request="BYE" dialog="04">\n   <\![CDATA[\n      REGISTER sip:[service]@[remote_ip]:[remote_port] SIP/2.0\n      cseq: [cseq dialog="01"]\n      Content-Length: [len]\n    ]]>\n  </send>\n  \n  <send request="SUBSCRIBE" dialog="02">\n   <\![CDATA[\n      REGISTER sip:[service]@[remote_ip]:[remote_port] SIP/2.0\n      Content-Length: [len]\n    ]]>\n  </send>\n  \n  \n  <\!-- include file in current sub-directory, replacement is aA=>99, Bb=>1, cc=>3 -->\n  \n  <\!-- include_substitution_1 file has directly specified parameters -->\n  \n  <\!-- include_substitution_2 file in sub-directory, inherits substitution -->\n  \n  <send request="INVITE" dialog="01">\n   <\![CDATA[\n      REGISTER sip:[service]@[remote_ip]:[remote_port] SIP/2.0\n      Content-Length: [len]\n    ]]>\n  </send>\n\n  <send request="BYE" dialog="99">\n   <\![CDATA[\n      REGISTER sip:[service]@[remote_ip]:[remote_port] SIP/2.0\n      cseq: [cseq dialog="01"]\n      Content-Length: [len]\n    ]]>\n  </send>\n  \n\n\n\n\n  \n\n  <send request="INVITE" dialog="99">\n   <\![CDATA[\n      REGISTER sip:[service]@[remote_ip]:[remote_port] SIP/2.0\n      Content-Length: [len]\n    ]]>\n  </send>\n\n  <send request="BYE" dialog="01">\n   <\![CDATA[\n      REGISTER sip:[service]@[remote_ip]:[remote_port] SIP/2.0\n      cseq: [cseq dialog="99"]\n      Content-Length: [len]\n    ]]>\n  </send>\n  \n  <send request="SUBSCRIBE" dialog="03">\n   <\![CDATA[\n      REGISTER sip:[service]@[remote_ip]:[remote_port] SIP/2.0\n      Content-Length: [len]\n    ]]>\n  </send>\n  \n\n</scenario>\n\n\n\n!
    assert(test.run())
  end

end
