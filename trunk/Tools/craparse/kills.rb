class Kills
	def initialize
	end

	def Kills.blaster()	
		File.open($file) do |op|
			op.grep(/was blasted by #{$player}/)
		end
	end

	def Kills.chaingunBurst()
		File.open($file) do |op|
			op.grep(/was blown away by #{$player}'s chaingun burst/)
		end
	end

	def Kills.chaingun()
		File.open($file) do |op|
			op.grep(/was cut in half by #{$player}'s chaingun/)
		end
	end

	def Kills.flame()
		File.open($file) do |op|
			op.grep(/was burned by #{$player}'s napalm/)
		end
	end

	def Kills.rocket()
		File.open($file) do |op|
			op.grep(/ate #{$player}'s rocket/)
		end
	end
	
	def Kills.rocketSplash()
		File.open($file) do |op|
			op.grep(/almost dodged #{$player}'s rocket/)
		end
	end

	def Kills.beamgun()
		File.open($file) do |op|
			op.grep(/was melted by #{$player}'s beamgun/)
		end
	end

	def Kills.disruptor()
		File.open($file) do |op|
			op.grep(/was disrupted by #{$player}/)
		end
	end

	def Kills.smartgun()
		File.open($file) do |op|
			op.grep(/saw the pretty lights from #{$player}'s smartgun/)
		end
	end

	def Kills.vaporizer()
		File.open($file) do |op|
			op.grep(/was disintegrated by #{$player}'s vaporizer blast/)
		end
	end

	def Kills.vaporizerAlt()
		File.open($file) do |op|
			op.grep(/couldn't hide from #{$player}'s vaporizer/)
		end
	end

	def Kills.plasmaSplash()
		File.open($file) do |op|
			op.grep(/was melted by #{$player}'s plasma/)
		end
	end

	def Kills.telefrag()
		File.open($file) do |op|
			op.grep(/tried to invade #{$player}'s personal space/)
		end
	end

	def Kills.grapple()
		File.open($file) do |op|
			op.grep(/was caught by #{$player}'s grapple/)
		end
	end

	def Kills.all()
		Kills.blaster().length + Kills.chaingunBurst().length + Kills.chaingun().length + Kills.flame().length + Kills.rocket().length + Kills.rocketSplash().length + Kills.beamgun().length + Kills.disruptor().length + Kills.smartgun().length + Kills.vaporizer().length + Kills.vaporizerAlt().length + Kills.plasmaSplash().length + Kills.telefrag().length + Kills.grapple().length
	end
end
