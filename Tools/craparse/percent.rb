require "kills.rb"

class Percent
	def initialize
	end

	def Percent.blaster()
		(Kills.blaster().length.to_f / Kills.all.to_f) * 100
	end

	def Percent.chaingunBurst()
		(Kills.chaingunBurst().length.to_f / Kills.all.to_f) * 100
	end

	def Percent.chaingun()
		(Kills.chaingun().length.to_f / Kills.all.to_f) * 100
	end

	def Percent.flame()
		(Kills.flame().length.to_f / Kills.all.to_f) * 100
	end

	def Percent.rocket()
		(Kills.rocket().length.to_f / Kills.all.to_f) * 100
	end

	def Percent.rocketSplash()
		(Kills.rocketSplash().length.to_f / Kills.all.to_f) * 100
	end

	def Percent.beamgun()
		(Kills.beamgun().length.to_f / Kills.all.to_f) * 100
	end

	def Percent.disruptor()
		(Kills.disruptor().length.to_f / Kills.all.to_f) * 100
	end

	def Percent.smartgun()
		(Kills.smartgun().length.to_f / Kills.all.to_f) * 100
	end

	def Percent.vaporizer()
		(Kills.vaporizer().length.to_f / Kills.all.to_f) * 100
	end

	def Percent.vaporizerAlt()
		(Kills.vaporizerAlt().length.to_f / Kills.all.to_f) * 100
	end

	def Percent.plasmaSplash()
		(Kills.plasmaSplash().length.to_f / Kills.all.to_f) * 100
	end

	def Percent.telefrag
		(Kills.telefrag().length.to_f / Kills.all.to_f) * 100
	end

	def Percent.grapple
		(Kills.telefrag().length.to_f / Kills.all.to_f) * 100
	end
end
