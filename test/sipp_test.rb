#!/usr/bin/ruby
#
# Copyright (c) 2010, Polycom, Inc.
#
# Author: Edward Estabrook <edward.estabrook@polycom.com>
#

require 'rubygems'
require 'getopt/std'
require 'English'

#
# Todo:
# - Fix for Windows
# - better integration of console output validation, don't output [PASS] before post-run validations
# - command-line options for logging, screen verbosity, ports, etc
# - pass params to initialize via hash for more flexibility
# - catch server-startup failure in client
# - detect non-existence of sipp in path for better error reporting
# - recursively kill sub-children of sipp in case invoked script performed exec (ie pcap_play)
#   [see http://t-a-w.blogspot.com/2010/04/how-to-kill-all-your-children.html]

class SippTest
  attr_accessor :client_options, :server_options, :logging, :expected_client_output, :expected_server_output
  
  def initialize(name, client_options, server_options = '')
    @name = name
    @client_options = client_options
    @server_options = server_options
    @sipp_local_port = 5069
    @sipp_remote_port = 15060
    @sipp_logging_parameters = "" # "-trace_screen -trace_msg"
    @sipp_path = "../sipp"
    @logging = "normal" # silent, normal, verbose
    @error_message = "";
    @server_screen_destination = "server_console.out" # "#{@name}_server.out"
    @client_screen_destination = "client_console.out" # "#{@name}_client.out"

    @server_pid = -1
    @server_aborted = false

  end

  def run
    print "Test #{@name} " unless @logging == "silent"
    print "\n" unless @logging != "verbose"

    start_sipp_server(server_commandline)

    success = start_sipp_client(client_commandline)

    stop_sipp_server()

    if (success)
      success = post_execution_validation()
    end

    puts result_message(success) unless @logging == "silent"
    
    return success;
  end

  def server_commandline
    return "#{@sipp_path} #{@server_options} -i 127.0.0.1 -p #{@sipp_remote_port} #{@sipp_logging_parameters} &> #{@server_screen_destination}"
  end

  def client_commandline
    return "#{@sipp_path} #{@client_options} -i 127.0.0.1 -p #{@sipp_local_port}  #{@sipp_logging_parameters} 127.0.0.1:#{@sipp_remote_port} &> #{@client_screen_destination}"
  end

  def post_execution_validation
    result = true
	if (!@expected_client_output.nil?)
      if (@expected_client_output != get_client_output())
	    puts "Expected client output does not match actual.\n" unless @logging == "silent"
		puts "Expected = '#{@expected_client_output}'\nActual = '#{get_client_output()}'\n" if @logging == "verbose"
		result = false;
	  end
	end
	if (!@expected_server_output.nil?)
      if (@expected_server_output != get_server_output())
	    puts "Expected server output does not match actual.\n" unless @logging == "silent"
		puts "Expected = '#{@expected_server_output}'\nActual = '#{get_server_output()}'\n" if @logging == "verbose"
		result = false;
	  end
	end
	
   #override to perform any additional follow-up tests here.
   return result
  end

  def start_sipp_client(testcase_client)
    success = false;
    puts "Executing client with '#{testcase_client}'" unless @logging != "verbose"

    if (!system(testcase_client))
      if ($CHILD_STATUS.exitstatus == -1)
        @error_message = "[ERROR] - Failed to execute"
      elsif ($CHILD_STATUS.exitstatus & 127)
        @error_message =  "[ERROR] - child died with signal #{$CHILD_STATUS.exitstatus & 127}"
      elsif ($CHILD_STATUS.exitstatus != 0)
        @error_message =  "[FAIL] exited with value #{$CHILD_STATUS.exitstatus >> 8}"
      end
    else
      success = true;
    end
    return success
  end

  def result_message(success)
    if (success)
      return "[PASS]"
    else
      return @error_message
    end
  end
  
  def get_client_output()
    return IO.read(@server_screen_destination)
  end

  def get_client_output()
    return IO.read(@client_screen_destination)
  end
  
  # run server sipp process in background, saving pid
  def start_sipp_server(testcase_server)
    @server_options.empty? and return false
    puts "Executing server with '#{testcase_server}'" unless @logging != "verbose"

    @server_pid = fork do
      if (!exec(testcase_server))
        puts "[ERROR] - Failed to execute server command '#{testcase_server}'"
# if this fails we need to know so that test fails, otherwise client might just run forever...
        @server_aborted = true
        return false
      end
    end
  end #start_sipp_server

  def stop_sipp_server
    # kill background SIPp server if it was started
    @server_options.empty? and return false

    # kill immediate children of the shell whose pid is stored in @server_pid
    # may want to
#    Hash[*`ps -f`.scan(/\s\d+\s/).map{|x|x.to_i}].each{ |pid,ppid|
    `ps -f`.each{|s|
      a = s.split()
      if (a[2].to_i == @server_pid)
        puts "Killing #{a[1]} because its ppid of #{@server_pid} matches the server process.\n" unless @logging != "verbose"
        Process.kill("SIGINT", a[1].to_i)
      end
    }

    Process.wait(@server_pid)

  end # stop_sipp_server
  
# helper routine for implementing test cases
# outputs a string for assigning to a string variable in code for validating sipp output
# to use, add the following to your test *before* to output a string that will pass when 
# specified as the expected_client_output parameter (manually verify test first time!)
#	client_output = test.get_client_output()
#	test.puts_escaped_string(client_output)
 
  def puts_escaped_string(astring)
	output = astring.gsub("\n", "\\n")
	output.gsub!("\r", "\\r")
	output.gsub!("!", "\\!")
	puts "%Q!#{output}!"
  end

end # class SippTest

