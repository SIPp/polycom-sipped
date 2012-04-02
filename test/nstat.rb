#!/usr/bin/ruby
# windows only version  
lastresult =""
result = `netstat -naob | grep -B1 "sipp.exe" `
run = true
while run
	lastresult = result
	result = `netstat -naob | grep -B1 "sipp.exe" `
	if (result != lastresult)
    ts = Time.new
		puts "-------------------------------------#{ts.inspect}"
		puts result
	end
	sleep 0.3
end

