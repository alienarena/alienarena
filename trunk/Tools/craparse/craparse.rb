#!/usr/bin/env ruby

$file = './qconsole.log'
$player = 'Ghidora'

require "kills.rb"

def get_players()
	File.open($file) do |op|
		op.grep(/entered the game/).inject([]) {|mem, line| mem << line.gsub(/ entered the game/, '') }.uniq
	end
end

def sortArrays()
	a0 = Compare.new(Kills.blaster().to_s)
	a1 = Compare.new(Kills.chaingunBurst().to_s)
	a2 = Compare.new(Kills.chaingun().to_s)
	a3 = Compare.new(Kills.flame().to_s)
	a4 = Compare.new(Kills.rocket().to_s)
	a5 = Compare.new(Kills.rocketSplash().to_s)
	a6 = Compare.new(Kills.beamgun().to_s)
	a7 = Compare.new(Kills.disruptor().to_s)
	a8 = Compare.new(Kills.smartgun().to_s)
	a9 = Compare.new(Kills.vaporizer().to_s)
	a10 = Compare.new(Kills.vaporizerAlt().to_s)
	a11 = Compare.new(Kills.plasmaSplash().to_s)
	a12 = Compare.new(Kills.telefrag().to_s)
	a13 = Compare.new(Kills.grapple().to_s)
	[a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13].sort_by { |a| a }
end

class Compare < String
	include Comparable
	def <=>(other)
		self.to_i <=> other.to_i
	end
	def to_i
		self.scan(/\d+/)[0].to_s.to_i
	end
end
