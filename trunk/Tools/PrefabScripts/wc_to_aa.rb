GAMEDIR = 'C:\Games\Alien Arena 2007' # Do not add a trailing slash!

=begin

    WorldCraft 3.3 -> Alien Arena map file convertion script
    Copyright (C) 2007 Tony Jackson

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    Tony Jackson can be contacted at tonyj@cooldark.com
	
=end

require 'Find'

if $*.length == 0
	puts 'Convert WorldCraft 3.3 files to Alien Arena format (full path to textures)'
	puts ''
	puts 'Usage:'
	puts '  ruby wc_to_aa.rb <filename>'
	exit
end
if not FileTest.exists?($*[0])
	puts 'Unable to find file ' + $*[0]
	exit
end

puts 'Processing map ' + $*[0]

# Now create arrays of texture names and paths
textures = Array.new
paths = Array.new

Find.find(GAMEDIR+'\\data1\\textures\\') do |path|
	if FileTest.directory?(path)
		if File.basename(path)[0] == ?.
			Find.prune       # Don't look any further into this directory.
		else
			next
		end
	else
		if File.basename(path).split('.').last == 'tga'
			textures << File.basename(path).split('.').first
			paths << File.dirname(path).split(GAMEDIR+'\\data1\\textures\\').last
		end
	end
end

mapin = File.open($*[0], 'r')
mapout = File.open('_'+$*[0], 'w')
mapin.each do | line |
	line = line.split(' ')
	if line[0] == '(' and line[4] == ')'
		texture = line[15]
		index = textures.index(texture)
		if index == nil
			puts 'Warning: unable to find texture: ' + texture
		else
			line[15] = File.join(textures[index],paths[index])
		end
	end
	mapout.puts line.join(' ')
end
mapin.close
mapout.close
puts 'New map file written'