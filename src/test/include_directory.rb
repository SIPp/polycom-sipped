#!/usr/bin/ruby
#
# Copyright (c) 2010, Polycom, Inc.
#
# Author: Edward Estabrook <edward.estabrook@polycom.com>
#


require 'test/unit'
require './sipp_test'

class IncludeDirectory < Test::Unit::TestCase
  def test_include_directory
    test = SippTest.new("include_directory", "-sf include_directory/include_directory.sipp -mc -dump_xml -skip_rlimit")

    test.expected_client_output = %Q!<?xml version="1.0" encoding="ISO-8859-1" ?>\n<\!DOCTYPE scenario SYSTEM "sipp.dtd">\n\n<scenario name="include_directories" params="-mc" xmlns:xi="http://www.w3.org/2001/XInclude">\n\n  <\!-- include file in current sub-directory -->\n  <?xml version="1.0" encoding="ISO-8859-1" ?>\n<\!DOCTYPE scenario SYSTEM "sipp.dtd">\n<scenario name="Include Substitution 1" parameters="-mc" xmlns:xi="http://www.w3.org/2001/XInclude">\n  <\!-- include file in sub-directory -->\n  <?xml version="1.0" encoding="ISO-8859-1" ?>\n<\!DOCTYPE scenario SYSTEM "sipp.dtd">\n<scenario name="Include Substitution 1" parameters="-mc xmlns:xi="http://www.w3.org/2001/XInclude"">\n\n  <\!-- include file in parent directory -->\n  <?xml version="1.0" encoding="ISO-8859-1" ?>\n<\!DOCTYPE scenario SYSTEM "sipp.dtd">\n<scenario name="Include Substitution 1" parameters="-mc" xmlns:xi="http://www.w3.org/2001/XInclude">\n  <\!-- included from include_subdir -->\n  <send>\n   <\![CDATA[\n      INVITE sip:[service]@[remote_ip]:[remote_port] SIP/2.0\n      Content-Length: [len]\n    ]]>\n  </send>\n  \n</scenario>\n\n\n <\!-- sent message irrelevent, test passes if it loads -->\n  <send >\n    <\![CDATA[\n\n      REGISTER sip:[service]@[remote_ip]:[remote_port] SIP/2.0\n      Via: SIP/2.0/[transport] [local_ip]:[local_port];branch=[branch]\n\n    ]]>\n  </send>\n\n</scenario>\n\n\n</scenario>\n\n\n</scenario>\n!
    assert(test.run())
  end
  
  def test_include_directory_sequence_diagram
    test = SippTest.new("include_directory_sequence_diagram", "-sf include_directory/include_directory.sipp -mc -dump_sequence_diagram -skip_rlimit")
	
    test.expected_client_output = %Q!0  :          INVITE ----------> \r\n1  :        REGISTER ----------> \r\n!
    assert(test.run())
  end

end
