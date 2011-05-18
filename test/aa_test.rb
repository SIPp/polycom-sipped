#!/usr/bin/ruby
#
# Copyright (c) 2010, Polycom, Inc.
#
# Author: Edward Estabrook <edward.estabrook@polycom.com>
#


require 'test/unit'
require 'sipp_test'

class AaTest < Test::Unit::TestCase
  def test_aa
    test = SippTest.new("aa_test", "-sf aa_test_client.sipp -mc -aa", "-sf aa_test_server.sipp -mc")
    assert(test.run())
  end
end

