#!/usr/bin/ruby
#
# @author: richard lum
# @date: 20120328
#
# tests focused on confirming location reporting in source files when parsing errors occur
#
require 'test/unit'
require './sipp_test'


class WhereAmI < Test::Unit::TestCase
  #environment variable in xi include file should be bracketed by % eg %SIPPED%/includethisfile.xml 
  def test_badenv
    test = SippTest.new("test_badenv", "-sf include_incl_badenv.sipp")
    test.expected_error_log = /Malformed environment environment variable - missing closing %\s*\nFound at\:\s*\n\s*include_badenvvar.sipp\:7\s*\n\s*include_incl_badenv.sipp\:10/
    test.expected_exitstatus = 255
    #test.logging="verbose"
    assert(test.run())
  end
 
  #environment variable is not set and used in xi include
  def test_unset_env
    test = SippTest.new("test_unset_env", "-sf include_incl_unset_envvar.sipp") #unsetenv_error.txt
    test.expected_error_log = /Undefined Environment Variable \: TDIR\r*\nFound at\:\s*\n\s*include_unset_envvar.sipp\:10\s*\n.*include_incl_unset_envvar.sipp\:10/
    #test.logging="verbose"
    test.expected_exitstatus = 255
    assert(test.run())
  end
  
  #reference to a non conformant tag
  def test_badtag
    test = SippTest.new("test_badtag", "-sf include_incl_badtag.sipp")
    test.expected_error_log = /xi\:include tag must be formatted exactly .*\nFound at\:\s*\n\s*include_badtag.sipp\:10\s*\n\s*include_incl_badtag.sipp\:10/
    #test.logging="verbose"
    test.expected_exitstatus = 255
    assert(test.run())
  end
  
  #scenario.cpp triggers error call caused by non existant tag
  def test_falsetag
    test = SippTest.new("test_falsetag", "-sf include_madeuptag.sipp -mc")
    test.expected_error_log = /Unknown element \'nonexistant\' in xml scenario file\s*\n\s*madeuptag.xml\:9\s*\n\s*include_madeuptag.sipp\:7/
    #test.logging="verbose"
    test.expected_exitstatus = 255
    assert(test.run())
  end

  #scenario.cpp triggers error call due to no cdata
  def test_nocdata
    test = SippTest.new("test_nocdata", "-sf include_nocdata.sipp -mc")
    test.expected_error_log = /No CDATA in 'send' section of xml scenario file\ninclude_directory\/nocdata.xml\:15\ninclude_nocdata.sipp\:10/
    #test.logging="verbose"
    test.expected_exitstatus = 255
    assert(test.run())
  end
  
end
    
    