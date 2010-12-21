#!/usr/bin/ruby
#
# Copyright (c) 2010, Polycom, Inc.
#
# Author: Edward Estabrook <edward.estabrook@polycom.com>
#


require 'test/unit'
require 'sipp_test'

class AaExpiresSpecified < Test::Unit::TestCase
  def test_keywords
    test = SippTest.new("verify_keywords", "-sf verify_keywords_client.sipp -mc", "-sf verify_keywords_server.sipp -mc")
	test.logging = "verbose"
    assert(test.run())
  end
end
