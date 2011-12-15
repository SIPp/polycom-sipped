#!/usr/bin/ruby
#
# Copyright (c) 2010, Polycom, Inc.
#
# Author: Edward Estabrook <edward.estabrook@polycom.com>
#

require 'rubygems'
require 'getopt/std'
require 'English'
require 'rbconfig'
require 'optparse'
require 'win32/process' if Config::CONFIG["host_os"] =~ /mswin|mingw/
require 'sys/proctable' if Config::CONFIG["host_os"] =~ /mswin|mingw/

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
# - detect failure of server and stop test immediately and indicate failure
#
# Note use 'ruby test.rb --name test_name' to execute the test named 'test_name' in test file 'test.rb'

class SippTest
  attr_accessor :client_options, :server_options, :logging, 
                :expected_client_output, :expected_server_output, :expected_error_log,
                :expected_minimum_run_time, :expected_maximum_run_time, :run_time,
                :expected_exitstatus

  def initialize(name, client_options, server_options = '')
    process_args
    @is_windows = Config::CONFIG["host_os"] =~ /mswin|mingw/
    @name = name
    @client_options = client_options
    @server_options = server_options
    @sipp_local_port = 5069
    @sipp_remote_port = 15060
    @sipp_logging_parameters = "" # "-trace_screen -trace_msg"
    @sipp_path = (@is_windows)? "..\\sipp.exe" : "../sipp"
    @logging = "normal" unless @logging
    @error_message = "";
    @server_screen_destination = "server_console.out" # "#{@name}_server.out"
    @client_screen_destination = "client_console.out" # "#{@name}_client.out"
    @error_log_destination  = "error.log"

    @server_pid = -1
    @server_aborted = false

    @run_time = 0;
    @expected_exitstatus = 0;
    
    @to_output = (@is_windows)? ">" : "&>"
    @redirect_error = (@is_windows)? " 2>&1" : ""

  end

  def run
    print "Test #{@name} " unless @logging == "silent"

    start_time = Time.now

    start_sipp_server(server_commandline)

    success = start_sipp_client(client_commandline)

    stop_sipp_server()

    @run_time = Time.now - start_time

    if (success)
      success = post_execution_validation()
    end

    puts "#{result_message(success)}\n" unless @logging == "silent"

    return success;
  end

  def server_commandline
    return "#{@sipp_path} #{@server_options} -i 127.0.0.1 -p #{@sipp_remote_port} #{@sipp_logging_parameters} 127.0.0.1:#{@sipp_local_port}" + @to_output + "#{@server_screen_destination}" + @redirect_error
  end

  def client_commandline
    if(@expected_error_log.nil?)
      return "#{@sipp_path} #{@client_options} -i 127.0.0.1 -p #{@sipp_local_port}  #{@sipp_logging_parameters} 127.0.0.1:#{@sipp_remote_port}" + @to_output + "#{@client_screen_destination}" + @redirect_error
    else
      return "#{@sipp_path} #{@client_options} -trace_err -error_file error.log -i 127.0.0.1 -p #{@sipp_local_port}  #{@sipp_logging_parameters} 127.0.0.1:#{@sipp_remote_port}" + @to_output + "#{@client_screen_destination}" + @redirect_error
    end
  end

  def post_execution_validation
    result = true

    if (@is_windows)
      remove_carriage_returns()
    end

    #Note that expected_client/server_output use strings, whereas expected_error_log uses a regular expression.
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

    if(!@expected_error_log.nil?)
      if !(@expected_error_log.match get_error_log)
        puts "Expected error log does not match actual.\n" unless @logging == "silent"
        puts "Expected = '#{@expected_error_log}'\nActual = '#{get_error_log()}'\n" if @logging == "verbose"
    result = false;
      end
    end
        
    if (!@expected_minimum_run_time.nil?)
      if (@expected_minimum_run_time > @run_time)
        puts "Run time #{@run_time} is less than expected minimum of #{@expected_minimum_run_time}.\n" unless @logging == "silent"
        result = false;
      end
    end

    if (!@expected_maximum_run_time.nil?)
      if (@expected_maximum_run_time < @run_time)
        puts "Run time #{@run_time} is greater than expected maximum of #{@expected_maximum_run_time}.\n" unless @logging == "silent"
          result = false;
      end
    end

    #override to perform any additional follow-up tests here.
    return result
  end

  def start_sipp_client(testcase_client)
    success = false;
    puts "Executing client with '#{testcase_client}'" if @logging == "verbose"

    result = system(testcase_client)
    print "result = #{result} ; exitstatus = #{$CHILD_STATUS.exitstatus} ; expecting #{@expected_exitstatus}\n" if @logging == "verbose"

    if ( (result && @expected_exitstatus == 0) || !expected_error_log.nil? ) 
      success = true
    elsif ($CHILD_STATUS.exitstatus == -1)
      @error_message = "[ERROR] - Failed to execute"
    elsif ($CHILD_STATUS.signaled?)
      @error_message =  "[ERROR] - child died with signal #{$CHILD_STATUS.termsig}]"
    elsif ($CHILD_STATUS.exited?)
      if ($CHILD_STATUS.exitstatus != @expected_exitstatus)
    @error_message =  "[FAIL] exited with value #{$CHILD_STATUS.exitstatus} while expecting #{@expected_exitstatus}."
      else
        success = true
      end
    else 
      @error_message = "Unknown failure: #{$CHILD_STATUS.to_s}"
    end
  return success
  end

  def result_message(success)
    print "\nTest #{@name} " if @logging == "verbose"
    if (success)
      return "[PASS]"
    else
      return @error_message
    end
  end
  
  def get_server_output()
    return IO.read(@server_screen_destination)
  end

  def get_client_output()
    return IO.read(@client_screen_destination)
  end
  
  def get_error_log()
    return IO.read(@error_log_destination)
  end
  # run server sipp process in background, saving pid
  def start_sipp_server(testcase_server)
    @server_options.empty? and return false
    puts "Executing server with '#{testcase_server}'" if @logging == "verbose"
    if @is_windows
      @server_pid = IO.popen(testcase_server).pid
    else
      @server_pid = fork do
        if (!exec(testcase_server))
          puts "[ERROR] - Failed to execute server command '#{testcase_server}'"
          # if this fails we need to know so that test fails, otherwise client might just run forever...
          @server_aborted = true
          return false
        end
      end
    end
  end #start_sipp_server

  def stop_sipp_server
    # kill background SIPp server if it was started
    @server_options.empty? and return false

    if @is_windows
      Sys::ProcTable.ps.each { |ps|
        if ps.name.downcase == "sipp.exe"
          Process.kill('KILL', ps.pid)
        end
      }
      Process.kill("SIGINT", @server_pid)
    else
      # kill immediate children of the shell whose pid is stored in @server_pid
      # may want to
      # Hash[*`ps -f`.scan(/\s\d+\s/).map{|x|x.to_i}].each{ |pid,ppid|
      `ps -f`.each{|s|
        a = s.split()
        if (a[2].to_i == @server_pid)
          puts "Killing #{a[1]} because its ppid of #{@server_pid} matches the server process.\n" if @logging == "verbose"
          Process.kill("SIGINT", a[1].to_i)
        end
      }
    end
    Process.wait(@server_pid)

  end # stop_sipp_server

  def process_args

    optparse = OptionParser.new do|opts|
      # Set a banner, displayed at the top
      # of the help screen.
      opts.banner = "Usage: ruby test_case.rb [options]"
 
      # Define the $options, and what they do
      opts.on(["silent", "normal", "verbose"], '-v', '--verbose [LEVEL]', 'Set verbosity depending on LEVEL and enables Test::Unit::TestCase verbose output.
                                     If LEVEL is not specified default is "verbose".' ) do|level|
        @logging = level || "verbose"
      end

      # These are just for (hacked together) compatibility with Test::Unit options
      opts.on '-s', '--seed SEED', Integer, "Sets random seed" do
      end

      opts.on '-n', '--name PATTERN', "Filter test names on pattern." do
      end

      # This displays the help screen, all programs are
      # assumed to have this option.
      opts.on( '-h', '--help', 'Display this screen' ) do
        puts opts
        exit
      end
      opts.parse ARGV
    end

  end

  # helper routine for implementing test cases
  # outputs a string for assigning to a string variable in code for validating sipp output
  # to use, add the following to your test *before* to output a string that will pass when 
  # specified as the expected_client_output parameter (manually verify test first time!)
  #    test.run()
  #    client_output = test.get_client_output()
  #    test.puts_escaped_string(client_output)
# Note that test which compare output to must use -skip_rlimit flag to eliminate FD_SETSIZE errors
  def puts_escaped_string(astring)
    output = astring.gsub("\n", "\\n")
    output.gsub!("\r", "\\r")
    output.gsub!("!", "\\!")
    puts "%Q!#{output}!"
  end

  def remove_carriage_returns()
    if (!@expected_client_output.nil?)
      @expected_client_output = @expected_client_output.gsub("\r","")
    end
    if not @expected_server_output.nil?
      @expected_server_output = @expected_server_output.gsub("\r","")
    end
  end

  def remove_space_and_crlf(astring)
    output = astring.gsub("\r", "")
    output.gsub!("\n", "")
    output.gsub!(" ", "")    
    return output
  end

end # class SippTest

