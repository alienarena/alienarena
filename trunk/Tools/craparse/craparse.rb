#!/usr/bin/env ruby

#The file that we parse
$file = './qconsole.log'
#The player we are looking for stats is the first command-line argument to the script.
$player = ARGV[0]

#Actually parses $file for the amount of kills/deaths
require "kills.rb"
#A way to sort strings based on ONE integrated number. Used in sortArrays()
require "compareStrWithNum.rb"

#Parses log file for all player names, this is NOT used yet.
#It would be nice if no arguments were placed for $player to
#Use this to show stats for ALL players, hehe.
def get_players()
	File.open($file) do |op|
		op.grep(/entered the game/).inject([]) {|mem, line| mem << line.gsub(/ entered the game/, '') }.uniq
	end
end

#Creates & Sorts arrays of weapon kills.. Needs cleanup
def sortArrays()
	a0 = Compare.new("Blaster Kills:\t\t #{Kills.blaster().to_s}")
	a1 = Compare.new("Chaingun Burst Kills:\t #{Kills.chaingunBurst().to_s}")
	a2 = Compare.new("Chaingun Kills:\t\t #{Kills.chaingun().to_s}")
	a3 = Compare.new("Flamethrower Kills:\t #{Kills.flame().to_s}")
	a4 = Compare.new("Rocket Kills:\t\t #{Kills.rocket().to_s}")
	a5 = Compare.new("Rocket Splash Kills:\t #{Kills.rocketSplash().to_s}")
	a6 = Compare.new("Beamgun Kills:\t\t #{Kills.beamgun().to_s}")
	a7 = Compare.new("Disruptor Kills:\t #{Kills.disruptor().to_s}")
	a8 = Compare.new("Smartgun Kills:\t\t #{Kills.smartgun().to_s}")
	a9 = Compare.new("Vaporizer Kills:\t #{Kills.vaporizer().to_s}")
	a10 = Compare.new("Vaporizer Bomb Kills:\t #{Kills.vaporizerAlt().to_s}")
	a11 = Compare.new("Plasma Splash Kills:\t #{Kills.plasmaSplash().to_s}")
	a12 = Compare.new("Telefrag Kills:\t\t #{Kills.telefrag().to_s}")
	a13 = Compare.new("Grapple Kills:\t\t #{Kills.grapple().to_s}")
	[a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13].sort_by { |a| a }
end

#By default Compare sorts by least to greatest, lets reverse that to greatest to least.
puts sortArrays().reverse
