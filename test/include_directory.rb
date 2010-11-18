#!/usr/bin/ruby
#
# Copyright (c) 2010, Polycom, Inc.
#
# Author: Edward Estabrook <edward.estabrook@polycom.com>
#


require 'test/unit'
require 'sipp_test'

class IncludeDirectory < Test::Unit::TestCase
  def test_expires
    test = SippTest.new("include_directory", "-sf include_directory/include_directory.sipp -m 1 -l 1")
    assert(test.run())
  end
end
