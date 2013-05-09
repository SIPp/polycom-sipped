#!/usr/bin/ruby
#
# Copyright (c) 2010, Polycom, Inc.
#
# Author: Edward Estabrook <edward.estabrook@polycom.com>
#

require 'rubygems'
#require 'getopt/std'
require 'English'
require 'rbconfig'
require 'optparse'
require 'set'
require 'win32/process' if RbConfig::CONFIG["host_os"] =~ /mswin|mingw/
require 'sys/proctable' if RbConfig::CONFIG["host_os"] =~ /mswin|mingw/

#
# Todo:
# - Fix for Windows
# - better integration of console output validation, don't output [PASS] before post-run validations
# - command-line options for logging, screen verbosity, ports, etc
# - pass params to initialize via hash for more flexibility
# - detect non-existence of sipp in path for better error reporting
# - recursively kill sub-children of sipp in case invoked script performed exec (ie pcap_play)
#   [see http://t-a-w.blogspot.com/2010/04/how-to-kill-all-your-children.html]
# - detect failure of server and stop test immediately and indicate failure
#
# Note use 'ruby test.rb --name test_name' to execute the test named 'test_name' in test file 'test.rb'
#   def 'test_name' not SippTest.new("name" ...
# For verbose output 'ruby test.rb --name test_name -- -v' to tell ruby to pass -v arg onto test.rb

# WARNING: this test suite requires exclusive use of PORT 5069 and 15060 but you can run
# other sipp instances in parallel on other ports.

class SippTest
  attr_accessor :client_options, :server_options, :logging, 
                :expected_client_output, :expected_server_output, :expected_error_log,
                :expected_minimum_run_time, :expected_maximum_run_time, :run_time,
                :expected_exitstatus
  attr_reader   :error_message

  def initialize(name, client_options, server_options = '')
    process_args
    @is_windows = RbConfig::CONFIG["host_os"] =~ /mswin|mingw/
    @name = name
    @client_options = client_options
    @server_options = server_options 
    @sipp_local_port = 5069
    @sipp_remote_port = 15060
    @sipp_logging_parameters = "" #"-trace_debug"  #" -trace_screen -trace_msg"
    #@sipp_path = (@is_windows)? "..\\Debug\\sipp.exe" : "../sipp"
    @sipp_path = (@is_windows)? "..\\sipp.exe" : "../sipp" # This means sipp.exe must get copied from Debug folder in win32 platform
    @logging = "normal" unless @logging
    @error_message = "";
    @server_screen_destination =  "#{@name}_server.out"
    @client_screen_destination =  "#{@name}_client.out"
    @error_log_destination  = "#{@name}_error.log"

    @server_pid = -1
    @server_aborted = false

    @run_time = 0;
    @expected_exitstatus = 0;
    
    @to_output = (@is_windows)? ">" : "&>"
    @redirect_error = (@is_windows)? " 2>&1" : ""

  end

  def netstat(grep_value)
    if (@is_windows) 
      # netstat option o is for process that owns connection
      cmd="netstat -nao | grep \"LISTEN\\|ESTABLISHED\\|UDP\" | grep \"#{grep_value}\""   
    else
      # netstat option p is for pid/program running connecion
      cmd="netstat -nap | grep -i \"LISTEN\\|ESTABLISHED\\|UDP\" | grep \"#{grep_value}\""
    end
    result = `#{cmd}`
    puts "netstat_command: '#{cmd}'\n'#{result}'" if @logging=="verbose"
    return result
  end
  
  def netstat_remote_ports
    return netstat(@sipp_remote_port)
  end

  def netstat_remote_tcp_ports
    # netstat | grep port | grep -i TCP
    return netstat("#{@sipp_remote_port}\" | grep -i \"TCP")
  end
  
  def netstat_local_and_remote_ports
    return netstat("#{@sipp_remote_port}\\|#{@sipp_local_port}")
  end
  

  def is_remote_port_in_use
    result = netstat_remote_ports()
    bool = result.include? "#{@sipp_remote_port}"
    puts "is_remote_port_in_use(): result.include? #{@sipp_remote_port} = #{bool}" if @logging == "verbose"
    return bool
  end
  

  #kills any process sitting on local or remote test port
  #   cannot use stop_sipp_server since that logic is based upon
  #   the server_pid of the previous instance of sipp_test and is no longer available.
  def kill_sipp_processes
    puts "kill_sipp_process: Find pid of process blocking our port"
    # result will not contain any TIMEWAIT states
    result = netstat_local_and_remote_ports()

    if (not (@is_windows))
      #kill processes holding  our ports
      result.each{|s|
        a = s.split()
        
        # examples of all forms of netstat output, note variable number of arguments
        # missing state field means pid is arg 5 instead of 6
        # 0          1      2 3                           4                           5           6
        # Proto Recv-Q Send-Q Local Address               Foreign Address             State       PID/Program name  
        # tcp        0      0 127.0.0.1:15060             0.0.0.0:*                   LISTEN      2087/sipp  
        # udp        0      0 127.0.0.1:15060             127.0.0.1:5069              ESTABLISHED 5460/sipp
        # udp        0      0 127.0.0.1:15060             0.0.0.0:*                               26852/sipp  
        # udp        0      0 :::15060                    :::* 
        # last case we cannot id process blocking our port, 
               
        # pid may be in arg 5 or 6 of each line.
        for argno in (5..6) do
          if (a[argno])
              pos=a[argno].index('/')
              if (pos) 
                #rely on fact that arg 5 or 6 will contain pid/process
                puts s if @logging == "verbose"
                pidstr=a[argno].to_s
                pidstr = pidstr[0,pos]
                pid = pidstr.to_i
                puts "WARNING: PORT BLOCKED,  sigkill pid #{pid} : #{a[3]}  #{a[argno]}" 
                Process.kill("SIGKILL",pid)
              else
                puts "ERROR: Unable to obtain PID from netstat: cannot kill SIPp process.\n"
              end
          end
        end 
      }
    else #is_windows
      #  Again, note variable number of arg possible in ea line
      #  0      1                      2                      3               4
      #  Proto  Local Address          Foreign Address        State           PID
      #  TCP    172.23.2.49:50693      172.23.0.200:389       ESTABLISHED     988
      #  UDP    127.0.0.1:5069         *:*                                    133712
      #  UDP    127.0.0.1:15060        *:*                                    133568

      #  Note on win7 UDP state rarely has a value, even when it is LISTENING
      #     cannot rely state as an indicator for kill candidate 
      result.each{|s|
        a = s.split()
        # pid may be in arg 3 or 4 of each line
        for argno in (3..4) do
          if ((a[argno])&&(a[argno].to_i>0))
              # rely on fact that a number in arg 3 or 4 is going to be a pid
              pid = a[argno].to_i
              puts "WARNING: PORT BLOCKED,  sigkill pid #{pid} : #{a[0]}  #{a[1]}  #{a[2]}" 
              Process.kill("SIGKILL",pid)
          else
            puts "ERROR: Unable to obtain PID from netstat: cannot kill SIPp process.\n"
          end
        end
      } 
    end #is_windows
  end
 
  # Ensure the server is up on port before starting client side
  # return success indicator
  def wait_for_server_to_be_ready
    puts "wait_for_server_to_be_ready()\n" if @logging == "verbose"
    max_wait_time = 4
    time_waited = 0
    while ( (!is_remote_port_in_use()) && (time_waited < max_wait_time) )
        puts "wait_for_server_to_be_ready: waiting for server to start (waited #{time_waited} of maximum #{max_wait_time} seconds)." if @logging == "verbose"
        sleep 1
        time_waited += 1
    end

    if time_waited >= max_wait_time
       return false
    end

    return true
  end

  # At start of test, don't want any residual sipp processes from previous test to interfere
  def ensure_needed_sipp_ports_are_unused
    puts "ensure_needed_sipp_ports_are_unused()\n" if @logging == "verbose"
    maximum_wait_time = 8
    time_waited_for_server = 0
    while(is_remote_port_in_use())
      puts "Waiting for previous test server to finish (waited #{time_waited_for_server} of maximum #{maximum_wait_time} seconds)." if @logging == "verbose"
      sleep 1
      time_waited_for_server += 1
      if (time_waited_for_server >= maximum_wait_time)
        puts "Previous SIPp test server taking too long: will kill it now." unless @logging == "silent"
        kill_sipp_processes()
      end
    end
  end

##############################  
## main entry point and test routines

  def run
    print "\nTest #{@name} \n" unless @logging == "silent"
    ensure_needed_sipp_ports_are_unused()  
    start_time = Time.now
    success = true

    if (need_sipp_server())
      success = start_sipp_server(server_commandline)
      if ((success) and (!wait_for_server_to_be_ready()))
        puts "Error: Server not ready for use: stopping test case\n" unless @logg
        stop_sipp_server()
        success = false
      end
    end

    if (success )
      success = start_sipp_client(client_commandline)
      stop_sipp_server()
    end

    @run_time = Time.now - start_time
    if (success)
      success = post_execution_validation()
    end

    print "\nTest #{@name} " if @logging == "verbose" # print again as all detail obscures which test it is...
    print "#{result_message(success)}\n" unless @logging == "silent"

    return success;
  end

  def server_commandline
    return "#{@sipp_path} #{@server_options} -i 127.0.0.1 -p #{@sipp_remote_port} #{@sipp_logging_parameters} 127.0.0.1:#{@sipp_local_port}" + @to_output + "#{@server_screen_destination}" + @redirect_error
  end

  def client_commandline
    if(@expected_error_log.nil?)
      return "#{@sipp_path} #{@client_options} -i 127.0.0.1 -p #{@sipp_local_port}  #{@sipp_logging_parameters} 127.0.0.1:#{@sipp_remote_port}" + @to_output + "#{@client_screen_destination}" + @redirect_error
    else
      return "#{@sipp_path} #{@client_options} -trace_err -error_file #{@error_log_destination} -i 127.0.0.1 -p #{@sipp_local_port}  #{@sipp_logging_parameters} 127.0.0.1:#{@sipp_remote_port}" + @to_output + "#{@client_screen_destination}" + @redirect_error
    end
  end

  def post_execution_validation
    result = true

    if (@is_windows)
      remove_carriage_returns()
    end

    #Note that expected_client/server_output use strings, whereas expected_error_log uses a regular expression.
    if (!@expected_client_output.nil?)
      if (remove_space_and_crlf(@expected_client_output) != remove_space_and_crlf(get_client_output()))
        puts "Expected client output does not match actual.\n" unless @logging == "silent"
        puts "Expected = '#{@expected_client_output}'\nActual = '#{get_client_output()}'\n" if @logging == "verbose"
        result = false;
      end
    end

    if (!@expected_server_output.nil?)
      if (remove_space_and_crlf(@expected_server_output) != remove_space_and_crlf(get_server_output()))
        puts "Expected server output does not match actual.\n" unless @logging == "silent"
        puts "Expected = '#{@expected_server_output}'\nActual = '#{get_server_output()}'\n" if @logging == "verbose"
        result = false;
      end
    end

    if(!@expected_error_log.nil?)
      if (!@expected_error_log.match(get_error_log()))
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
      puts "RUNTIME = #{@run_time}" if @logging == "verbose"
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

    if (!result && @expected_exitstatus == 0)  
      puts "SIPp client returned false exitstatus = #{$CHILD_STATUS.exitstatus} and expected_exitstatus code 0 indicates test expects success." unless logging == "silent"
      success = false
    elsif ( (result && @expected_exitstatus == 0) || !expected_error_log.nil? ) 
      success = true
    elsif ($CHILD_STATUS.exitstatus == -1)
      @error_message = "[ERROR] - sipp client failed to execute"
    elsif ($CHILD_STATUS.signaled?)
      @error_message =  "[ERROR] - sipp client died with signal #{$CHILD_STATUS.termsig}]"
    elsif ($CHILD_STATUS.exited?)
      if ($CHILD_STATUS.exitstatus != @expected_exitstatus)
        @error_message =  "[FAIL] sipp client exited with value #{$CHILD_STATUS.exitstatus} while expecting #{@expected_exitstatus}."
      else
        success = true
      end
    else 
      @error_message = "Unknown failure executing sipp client: #{$CHILD_STATUS.to_s}"
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

  def get_server_output()
    return IO.read(@server_screen_destination)
  end

  def get_client_output()
    return IO.read(@client_screen_destination)
  end

  def get_error_log()
    return IO.read(@error_log_destination)
  end

  def need_sipp_server()
    @server_options.empty? and return false
    return true
  end

  # run server sipp process in background, saving pid
  def start_sipp_server(testcase_server)
    @server_options.empty? and return false
    puts "Executing server with '#{testcase_server}'" if @logging == "verbose"
    if @is_windows
      @server_pid = IO.popen(testcase_server).pid
      puts "Server PID is '#{@server_pid}'" if @logging == "verbose"
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
    return true
  end #start_sipp_server

  def stop_sipp_server
    # kill background SIPp server if it was started
    @server_options.empty? and return false

    if @is_windows
      # SIGINT on child processes does not terminate them, must use sigkill
      Sys::ProcTable.ps.each { |ps|
        if ((ps.name.downcase == "sipp.exe")&&(ps.ppid==@server_pid))
        #if ((ps.name.downcase == "sipped.exe")&&(ps.ppid==@server_pid))
            puts "SIGKILL #{ps.pid}  #{ps.name}"  if @logging == "verbose"
            #todo catch Process::Error: The handle is invalid.
            begin
              Process.kill('SIGKILL', ps.pid)
            rescue Process::Error
              puts "process #{ps.pid}  #{ps.name} died before we could sigkill" if @logging == "verbose"
            end
        end
      }
      puts "Test Cleanup: Shutting down sipp server: sigint server pid = #{@server_pid}" if @logging == "verbose"
      Process.kill("SIGINT", @server_pid)
    else
      # kill immediate children of the shell whose pid is stored in @server_pid
      # may want to
      # Hash[*`ps -f`.scan(/\s\d+\s/).map{|x|x.to_i}].each{ |pid,ppid|
      `ps -ef`.each{|s|
        a = s.split()
        if (a[2].to_i == @server_pid)
          puts "Test Cleanup, SIGINT #{a[1]} because its ppid of #{@server_pid} matches the server process.\n"  if @logging == "verbose"
          # note, occasional error ERSCH will occur if pid dies between ps and kill.(not a failure - timing issue of no consequence)
          # swallow it here to prevent sending as error to tester and carry on 
          begin
            Process.kill("SIGINT", a[1].to_i)
          rescue Errno::ESRCH => e
            puts "process #{a[1]} died before we could sigint" if @logging == "verbose"
          end
        end
      }
      #sleep 1 # 99% of linux cases, INT will shutdown process quickly (load dependant) and not invoke KILL
      # if still not dead, once more with feeling....
      `ps -ef`.each{|s|
        a = s.split()
        if (a[2].to_i == @server_pid)
          puts "NOT DEAD YET:SIGKILL #{a[1]} because its ppid of #{@server_pid} matches the server process.\n"  if @logging == "verbose"
          begin
            Process.kill("SIGKILL", a[1].to_i)
          rescue Errno::ESRCH => e
            puts "Process #{a[1].to_i} terminated before we could sigkill, so Kill returned ESRCH response (but this is OK).\n"
          end
        end
      }
    end
    if (@is_windows)
        puts "Wait() for server PID #{@server_pid}" if @logging == "verbose"
        Process.waitpid(@server_pid) # crashes on my ruby install if process is specified...
    else
        Process.wait(@server_pid, Process::WNOHANG)
    end
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

