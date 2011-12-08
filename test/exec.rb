#!/usr/bin/ruby
#
# Copyright (c) 2010, Polycom, Inc.
#
# Author: Edward Estabrook <edward.estabrook@polycom.com>
#

require 'test/unit'
require './sipp_test'

class Exec < Test::Unit::TestCase
  def test_exec_command
  	# verify <exec command=""> [runs fast, doesn't mind 123 exit code]
    test = SippTest.new("exec_command", "-sf exec_command.sipp -m 1 -l 1 -key command \"sleep 3 && echo test \"", "-sn uas -aa ")
	test.expected_maximum_run_time = 2.5
    assert(test.run())
	sleep 3 # be sure child process has finished before test case finishes so address is no longer in use.
  end
  
  def test_exec_verify_pass
	# verify <exec verify=""> with pass [runs slow, passes with 0 exit code]
    test = SippTest.new("exec_verify_pass", "-sf exec_verify.sipp -m 1 -l 1 -key command \"perl sleep_and_return_0.pl\"", "-sn uas -aa ")
	test.expected_minimum_run_time = 3
    assert(test.run())
  end
  
  def test_exec_verify_fail
	# verify <exec verify=""> with fail [runs slow, causes sipp to exit with failure code]
    test = SippTest.new("exec_verify_fail", "-sf exec_verify.sipp -m 1 -l 1 -key command \"perl sleep_and_return_123.pl\"", "-sn uas -aa ")
	test.expected_minimum_run_time = 3
	test.expected_exitstatus = 255
    assert(test.run())
  end
  
  def test_exec_logging
	# verify <exec command> with stdout & stderr logging. [log file to end up with output]
	atime = Time.now
    test = SippTest.new("test_exec_logging", "-sf exec_two_verifies.sipp -m 1 -l 1 -trace_exec -exec_file exec_output.log -key command \"echo #{atime.to_s}\" -trace_debug", "-sn uas -aa ")
    assert(test.run())
	# verify that exec_output.log  contains #{atime.to_s}
	data = File.read("exec_output.log")
	expected = test.remove_space_and_crlf("<exec> verify \"echo #{atime.to_s} >> exec_output.log 2>&1\"\n#{atime.to_s}\n<exec> verify \"echo #{atime.to_s} >> exec_output.log 2>&1\"\n#{atime.to_s}\n")
	assert(test.remove_space_and_crlf(data) == expected, "data == expected")
  end

  def test_exec_special_xml_character_conversion
	# verify special xml character conversion in exec
    test = SippTest.new("test_exec_special_xml_character_conversion", "-sf exec_xml_character_conversion.sipp -mc -trace_exec -exec_file exec_output.log", "-sn uas -aa ")
    assert(test.run())
	# verify that exec_output.log  contains #{atime.to_s}
	data = File.read("exec_output.log")
	expected_windows = test.remove_space_and_crlf("<exec> verify \"echo \"< > & '\" >> exec_output.log 2>&1\"\n\"< > & '\" \n")
	expected_linux = test.remove_space_and_crlf("<exec> verify \"echo \"< > & '\" >> exec_output.log 2>&1\"\n< > & ' \n")
	result = test.remove_space_and_crlf(data)
	assert((result == expected_linux) || (result == expected_windows), result + " == " + expected_linux)
  end

  def test_exec_uses_correct_path
	# verify <exec verify="rsipp.pl ... ">
    test = SippTest.new("exec_uses_correct_path", "-mc -sf exec_uses_correct_path", "-sn uas -aa ")
	test.expected_minimum_run_time = 3
    assert(test.run())
  end
end
