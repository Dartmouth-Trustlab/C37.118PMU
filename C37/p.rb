
command = []
header = []
data = []
config = []
File.open("output.txt", "r") do |file|
	while line = file.gets
		type = line.split(' ')
		if type[0] == 'Command' and type[1] == 'Frame'
			command.append(type[3].to_f)	
		elsif type[0] == 'Config' and type[1] == '-'
			config.append(type[2].to_f)
		elsif type[0] == 'Data' and type[1] == 'Frame'
			data.append(type[3].to_f)
		elsif type[0] == 'Header' and type[1] == 'Frame'
			header.append(type[3].to_f)
		end
	end
end
puts "command %.6f" % (command.inject(:+)/command.length)
puts "Header %.6f" % (header.inject(:+)/command.length)
puts "Data %.6f" % (data.inject(:+)/data.length)
puts "Config %.6f" % (config.inject(:+)/config.length)
