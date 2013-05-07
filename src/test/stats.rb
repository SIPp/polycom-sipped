#!/usr/bin/ruby

if ARGV.length != 0
  puts "usage:  pipe numbers into #{$0}"
  puts "      numbers can be int or float"
  puts "      multiple numbers per line is ok"
  puts "      any text is ignored"
  exit
end

input = gets
#puts input
count = 0
total = 0.0
max = 0.0
min = 99999999.0
sumofsquares=0.0
while (input)
array = input.split()

array.each{|elem|
  num = elem.to_f
  if ( num > 0 )
    total += num
    count += 1
    sumofsquares += num*num
    if (num >max)
      max = num
    end
    if (num < min)
      min = num
    end
  end
  #puts   " num = #{num}  count = #{count} min = #{min} max = #{max} total = #{total} "
}
input=gets
end

  puts   " count = #{count} min = #{min} max = #{max} total = #{total} "
  average = total.to_f / count.to_f
  puts   "average = #{average}"
  variance = sumofsquares/count.to_f  - (average*average)
  puts   "variance = #{variance}"
  stddev = Math.sqrt(variance)
  puts "standard deviation = #{stddev}"
