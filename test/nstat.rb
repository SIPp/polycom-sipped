#!/usr/bin/ruby

lastresult =""
result = `netstat -naob | grep "5060\\|5069" `
run = true
while run
	lastresult = result
	result = `netstat -naob | grep "5060\\|5069" `
	if (result != lastresult)
		puts "----------------------"
		puts result
	end
	sleep 0.3
end

