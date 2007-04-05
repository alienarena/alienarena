#!/usr/bin/env ruby

$file = './qconsole.log'
$player = 'Ghidora'

require "stats.rb"

def get_players()
	File.open($file) do |op|
		op.grep(/entered the game/).inject([]) {|mem, line| mem << line.gsub(/ entered the game/, '') }.uniq
	end
end