$h = Hash.new
$h['blaster'] = /was blasted by #{$player}/
$h['chaingunBurst'] = /was blown away by #{$player}'s chaingun burst/
$h['chaingun'] = /was cut in half by #{$player}'s chaingun/
$h['flame'] = /was burned by #{$player}'s napalm/
$h['rocket'] = /ate #{$player}'s rocket/
$h['rocketSplash'] = /almost dodged #{$player}'s rocket/
$h['beamgun'] = /was melted by #{$player}'s beamgun/
$h['disruptor'] = /was disrupted by #{$player}/
$h['smartgun'] = /saw the pretty lights from #{$player}'s smartgun/
$h['vaporizer'] = /was disintegrated by #{$player}'s vaporizer blast/
$h['vaporizerAlt'] = /couldn't hide from #{$player}'s vaporizer/
$h['plasmaSplash'] = /was melted by #{$player}'s plasma/
$h['telefrag'] = /tried to invade #{$player}'s personal space/
$h['grapple'] = /was caught by #{$player}'s grapple/

class Kills
	def initialize
	end

	def Kills.blaster()	
		File.open($file) do |op|
			op.grep($h['blaster']).length
		end
	end

	def Kills.chaingunBurst()
		File.open($file) do |op|
			op.grep($h['chaingunBurst']).length
		end
	end

	def Kills.chaingun()
		File.open($file) do |op|
			op.grep($h['chaingun']).length
		end
	end

	def Kills.flame()
		File.open($file) do |op|
			op.grep($h['flame']).length
		end
	end

	def Kills.rocket()
		File.open($file) do |op|
			op.grep($h['rocket']).length
		end
	end
	
	def Kills.rocketSplash()
		File.open($file) do |op|
			op.grep($h['rocketSplash']).length
		end
	end

	def Kills.beamgun()
		File.open($file) do |op|
			op.grep($h['beamgun']).length
		end
	end

	def Kills.disruptor()
		File.open($file) do |op|
			op.grep($h['disruptor']).length
		end
	end

	def Kills.smartgun()
		File.open($file) do |op|
			op.grep($h['smartgun']).length
		end
	end

	def Kills.vaporizer()
		File.open($file) do |op|
			op.grep($h['vaporizer']).length
		end
	end

	def Kills.vaporizerAlt()
		File.open($file) do |op|
			op.grep($h['vaporizerAlt']).length
		end
	end

	def Kills.plasmaSplash()
		File.open($file) do |op|
			op.grep($h['plasmaSplash']).length
		end
	end

	def Kills.telefrag()
		File.open($file) do |op|
			op.grep($h['telefrag']).length
		end
	end

	def Kills.grapple()
		File.open($file) do |op|
			op.grep($h['grapple']).length
		end
	end

	def Kills.total()
		Kills.blaster() + Kills.chaingunBurst() + Kills.chaingun() + Kills.flame() + Kills.rocket() + Kills.rocketSplash() + Kills.beamgun() + Kills.disruptor() + Kills.smartgun() + Kills.vaporizer() + Kills.vaporizerAlt() + Kills.plasmaSplash() + Kills.telefrag() + Kills.grapple()
	end
end
