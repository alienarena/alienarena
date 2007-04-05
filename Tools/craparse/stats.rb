require "percent.rb"

class Stats
	def Stats.name()
		"Stats for " + $player
	end
	
	def Stats.allKills()
		"Total Kills: " + Kills.all().to_s
	end

	def Stats.blaster()
		"Blaster: \t\t\t" + sprintf("%.2f", Percent.blaster.to_s) + "% \t" + Kills.blaster.length.to_s + " kills"
	end

	def Stats.chaingunBurst()
		"Chaingun Burst: \t\t" + sprintf("%.2f", Percent.chaingunBurst.to_s) + "% \t" + Kills.chaingunBurst.length.to_s + " kills"
	end

	def Stats.chaingun()
		"Chaingun: \t\t\t" + sprintf("%.2f", Percent.chaingun.to_s) + "% \t" + Kills.chaingun.length.to_s + " kills"
	end

	def Stats.flame()
		"Flamethrower: \t\t\t" + sprintf("%.2f", Percent.flame.to_s) + "% \t" + Kills.flame.length.to_s + " kills"
	end

	def Stats.rocket()
		"Rocket Launcher: \t\t" + sprintf("%.2f", Percent.rocket.to_s) + "% \t" + Kills.rocket.length.to_s + " kills"
	end

	def Stats.rocketSplash()
		"Rocket Launcher Splash: \t" + sprintf("%.2f", Percent.rocketSplash.to_s) + "% \t" + Kills.rocketSplash.length.to_s + " kills"
	end
	
	def Stats.beamgun()
		"Beamgun: \t\t\t" + sprintf("%.2f", Percent.beamgun.to_s) + "% \t" + Kills.beamgun.length.to_s + " kills"
	end
	
	def Stats.disruptor()
		"Disruptor: \t\t\t" + sprintf("%.2f", Percent.disruptor.to_s) + "% \t" + Kills.disruptor.length.to_s + " kills"
	end

	def Stats.smartgun()
		"Smartgun: \t\t\t" + sprintf("%.2f", Percent.smartgun.to_s) + "% \t" + Kills.smartgun.length.to_s + " kills"
	end
	
	def Stats.vaporizer()
		"Vaporizer: \t\t\t" + sprintf("%.2f", Percent.vaporizer.to_s) + "% \t" + Kills.vaporizer.length.to_s + " kills"
	end

	def Stats.vaporizerAlt()
		"Vaporizer Bomb: \t\t" + sprintf("%.2f", Percent.vaporizerAlt.to_s) + "% \t" + Kills.vaporizerAlt.length.to_s + " kills"
	end

	def Stats.plasmaSplash()
		"Plasma Splash: \t\t\t" + sprintf("%.2f", Percent.plasmaSplash.to_s) + "% \t" + Kills.plasmaSplash.length.to_s + " kills"
	end

	def Stats.telefrag()
		"Telefrag: \t\t\t" + sprintf("%.2f", Percent.telefrag.to_s) + "% \t" + Kills.telefrag.length.to_s + " kills"
	end

	def Stats.grapple()
		"Grapple Hook: \t\t\t" + sprintf("%.2f", Percent.grapple.to_s) + "% \t" + Kills.grapple.length.to_s + " kills"
	end

	def Stats.sort()
		Stats.blaster().grep(/%/).to_s.split[2]
	end
end