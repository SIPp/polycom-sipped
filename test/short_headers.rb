#!/usr/bin/ruby
#
# Copyright (c) 2010, Polycom, Inc.
#
# Author: Edward Estabrook <edward.estabrook@polycom.com>
#


require 'test/unit'
require 'sipp_test'

class ShortHeaders < Test::Unit::TestCase
  def test_short_headers
    # Verify
    test1 = SippTest.new("verify_short_to_short", "-sf short_to_short_headers_client.sipp -mc", "-sf short_headers_server.sipp -mc")
    test2 = SippTest.new("verify_short_to_long", "-sf short_to_long_headers_client.sipp -mc", "-sf long_headers_server.sipp -mc")
    test3 = SippTest.new("verify_long_to_long", "-sf long_to_long_headers_client.sipp -mc", "-sf long_headers_server.sipp -mc")
    test4 = SippTest.new("verify_long_to_short", "-sf long_to_short_headers_client.sipp -mc", "-sf short_headers_server.sipp -mc")
    assert(test1.run())
    assert(test2.run())
    assert(test3.run())
    assert(test4.run())
  end
end

