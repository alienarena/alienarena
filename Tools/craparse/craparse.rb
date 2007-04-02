#!/usr/bin/env ruby

#This is the main file, it is to be minimalistic and simple.
#Should I put functions like get_players() and kills_makeStats() in
#funcs_misc.rb or something? Maybe I'll just leave 'em in.
#
#Todo: Deaths, more junk for Kills/Deaths. Clean up.. A LOT.

$file = './qconsole.log'
$player = 'Phenax'

require "kills.rb"

def get_players()
	File.open($file) do |op|
		$players = op.grep(/connected/) do |line|
			line.split[0]
		end
	end
end

def kills_makeStats()
	#We need more here, I'll do it once I get more "fundamental" features. This is also
	#an incredibly large formula which I hope can be shortened O_O..
	all_kills=Kills.blaster().length + Kills.chaingunBurst().length + Kills.chaingun().length + Kills.flame().length + Kills.rocket().length + Kills.rocketSplash().length + Kills.beamgun().length + Kills.disruptor().length + Kills.smartgun().length + Kills.vaporizer().length + Kills.vaporizerAlt().length + Kills.plasmaSplash().length + Kills.telefrag().length + Kills.grapple().length

	puts all_kills
end

kills_makeStats()
