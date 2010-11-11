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
# - command-line options for logging, screen verbosity, ports, etc
# - catch server-startup failure in client

class SippTest
  attr_accessor :client_options, :server_options
  
  def initialize(name, client_options, server_options)
    @name = name
    @client_options = client_options
    @server_options = server_options
    @sipp_local_port = 5069
    @sipp_remote_port = 15060
    @sipp_logging_parameters = "" # "-trace_screen -trace_msg"
    @sipp_path = "../sipp"
    @logging = "normal" # silent, normal, verbose
    @error_message = "";
    @server_screen_destination = "/dev/null" # {@name}_server.out
    @client_screen_destination = "/dev/null" # {@name}_client.out

    @server_pid = -1
    @server_aborted = false

  end

  def run
    print "Test #{@name} " unless @logging =="silent"

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
    #override to perform any additional follow-up tests here.
    return true
  end

  def start_sipp_client(testcase_client)
    success = false;
    if (@logging == "verbose")
      puts "Executing client with '#{testcase_client}'"
    end
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

  # run server sipp process in background, saving pid
  def start_sipp_server(testcase_server)
    if (@logging == "verbose")
      puts "Executing server with '#{testcase_server}'"
    end

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
    # kill background SIPp server.

    # ugly and won't work on windows...
    system("killall sipp")

    Process.wait(@server_pid)

  end # stop_sipp_server

end # class SippTest

