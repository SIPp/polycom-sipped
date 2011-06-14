#!/usr/bin/ruby
#
# Copyright (c) 2010, Polycom, Inc.
#
# Author: Edward Estabrook <edward.estabrook@polycom.com>
#


require 'test/unit'
require './sipp_test'

class AaExpiresSpecified < Test::Unit::TestCase
  def test_expires
    test = SippTest.new("aa_expires_specified", "-sf aa_expires_specified.sipp -m 1 -l 1", "-sn uas -aa -aa_expires 123")
    assert(test.run())
  end
end
