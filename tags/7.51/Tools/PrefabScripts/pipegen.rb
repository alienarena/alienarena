#!/usr/bin/env ruby

=begin
	Please read the licence agreement below
	
	These parameters let you edit the shape that will be generated
=end

FILE_TUBE = 'tube_wall.reg'
FILE_LIQUID = 'tube_liquid.reg'

BEND_ANGLE = 330			# total angle bend goes though
BEND_RADIUS = 128.0		# map units for tube bend radius
BEND_SEGMENTS = 22		# segments in the bend

TUBE_RADIUS  = 32.0		# map units for outside diameter of tube
TUBE_WALL = 4.0			# map units for wall thickness of tube
TUBE_SIDES = 16			# sides on tube cross-section

TUBE_TEXTURE = 		'martian/glass1'	# optional - tube wall texture
LIQUID_TEXTURE =	'arena8/slime' 		# optional - tube contents texture
LIQUID_FLOWING = 1 # 1 = flowing, 0 = static

# The following setting is currently not supported as for proper curves the rectangular brushes need to be subdivided into triangles
# It does work, but the resulting shape is not perfect, since it is made from rectangular brushs
BEND_OFFSET = 0			# if start and end are not in same plane, this defines the offset (spiral effect)

=begin

    RUBY PREFAB/REGION GENERATOR FOR RADIANT
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

require 'opengl'
require 'glut'

#FORMAT_STRING = "( %.6f %.6f %.6f ) "  # This is the precision used by AARadiant
FORMAT_STRING = "( %.10f %.10f %.10f ) " # This is the recision used by GtkRadiant

class Tube

  POS = 	[ 5.0,  5.0, 10.0,  0.0]
  RED = 	[ 0.8,  0.1,  0.0,  1.0]
  GREEN =	[ 0.0,  0.8,  0.2,  1.0]
  BLUE = 	[ 0.2,  0.2,  1.0,  1.0]

  include Math

  # from http://mathworld.wolfram.com/Torus.html
  def calc_tube_segment(tube_radius, bend_radius, sides, segment_angle, segment_height)
  	ring = Array.new
    for i in 0..sides
		vertex = Array.new
		tube_angle = i * 2.0 * PI / sides
		vertex[0] = ((bend_radius + tube_radius*cos(tube_angle))*sin(segment_angle));		# x
		vertex[1] = ((bend_radius + tube_radius*cos(tube_angle))*cos(segment_angle));		# y
		vertex[2] = (tube_radius*sin(tube_angle))+segment_height;							# z		
		ring << vertex;
    end
	return ring
  end

  def calc_sphere_segment(radius, sides, u)
  	ring = Array.new
    for i in 0..sides
		vertex = Array.new
		v = i * 2.0 * PI / sides
		
	    vertex[0] = cos(u)*sin(v);		# x
	    vertex[1] = sin(u)*sin(v);		# y
	    vertex[2] = cos(v);				# z

		ring << vertex;
    end
	return ring
  end

   # from http://mathworld.wolfram.com/Torus.html
  def calc_straight_segment(tube_radius, sides, z)
  	ring = Array.new
    for i in 0..sides
		vertex = Array.new
		tube_angle = i * 2.0 * PI / sides
		
	      vertex[0] = tube_radius*sin(tube_angle);		# x
	      vertex[1] = tube_radius*cos(tube_angle);		# y
	      vertex[2] = z;								# z		

		ring << vertex;
    end
	return ring
  end

  
	# Draw a tube
	def tube(tube_radius, bend_radius, sides, segments)
		GL.ShadeModel(GL::SMOOTH)
		#GL.ShadeModel(GL::FLAT)
		#GL.Normal(0.0, 0.0, 1.0)

		segment_outer = Array.new # outside of tube
		segment_inner = Array.new #inside of tube
		for i in 0..BEND_SEGMENTS
			height = i * BEND_OFFSET / BEND_SEGMENTS
			segment_outer << calc_tube_segment(tube_radius,				BEND_RADIUS, sides, i * BEND_ANGLE * 2.0 * PI / (BEND_SEGMENTS * 360), height)
			segment_inner << calc_tube_segment(tube_radius-TUBE_WALL,	BEND_RADIUS, sides, i * BEND_ANGLE * 2.0 * PI / (BEND_SEGMENTS * 360), height)
		end

		GL.Normal(0.0, 0.0, 1.0)
		for j in 0..(BEND_SEGMENTS-1)
			seg_angle=j * 2.0 * PI / segments
			GL.Begin(GL::QUADS)
			for i in 0..sides-1
				face_angle = i * 2.0 * PI / sides
#				GL.Normal(cos(face_angle), sin(face_angle), 0)
				#outside of tube
				GL.Normal(cos(face_angle)*cos(seg_angle), cos(face_angle)*sin(seg_angle), sin(face_angle));			
				GL.Vertex3f(segment_outer[j+1][i][0], 	segment_outer[j+1][i][1], 	segment_outer[j+1][i][2]);
				GL.Vertex3f(segment_outer[j][i][0], 	segment_outer[j][i][1], 	segment_outer[j][i][2]);
				GL.Vertex3f(segment_outer[j][i+1][0], 	segment_outer[j][i+1][1], 	segment_outer[j][i+1][2]);
				GL.Vertex3f(segment_outer[j+1][i+1][0], segment_outer[j+1][i+1][1],	segment_outer[j+1][i+1][2]);
#				GL.Normal(-cos(face_angle)*cos(seg_angle), -cos(face_angle)*sin(seg_angle), -sin(face_angle));		
				#inside of tube
				GL.Vertex3f(segment_inner[j][i+1][0], 	segment_inner[j][i+1][1], 	segment_inner[j][i+1][2]);
				GL.Vertex3f(segment_inner[j][i][0], 	segment_inner[j][i][1], 	segment_inner[j][i][2]);
				GL.Vertex3f(segment_inner[j+1][i][0], 	segment_inner[j+1][i][1], 	segment_inner[j+1][i][2]);
				GL.Vertex3f(segment_inner[j+1][i+1][0], segment_inner[j+1][i+1][1],	segment_inner[j+1][i+1][2]);
			end
			GL.End()
		end

		file = open(FILE_TUBE, 'w')
		file.puts '{'
		file.puts '"classname" "worldspawn"'

		for j in 0..(BEND_SEGMENTS-1)
			for i in 0..sides-1
				file.puts "// brush #{j*sides+i}"
				file.puts '{'
				entry = ''	# outer face
				entry.concat(vertex_string(segment_outer[j][i]))
				entry.concat(vertex_string(segment_outer[j+1][i]))
				entry.concat(vertex_string(segment_outer[j+1][i+1]))
				entry.concat(TUBE_TEXTURE)
				entry.concat(' 0 0 0 1 1 0 32 0')
				file.puts entry
				entry = '' # inner face
				entry.concat(vertex_string(segment_inner[j+1][i+1]))
				entry.concat(vertex_string(segment_inner[j+1][i]))
				entry.concat(vertex_string(segment_inner[j][i]))
				entry.concat(TUBE_TEXTURE)
				entry.concat(' 0 0 0 1 1 0 32 0')
				file.puts entry
				entry = '' #top face
				entry.concat(vertex_string(segment_inner[j][i]))
				entry.concat(vertex_string(segment_outer[j][i]))
				entry.concat(vertex_string(segment_outer[j][i+1]))
				entry.concat(TUBE_TEXTURE)
				entry.concat(' 0 0 0 1 1 0 32 0')
				file.puts entry
				entry = '' #bottom face
				entry.concat(vertex_string(segment_outer[j+1][i+1]))
				entry.concat(vertex_string(segment_outer[j+1][i]))
				entry.concat(vertex_string(segment_inner[j+1][i]))
				entry.concat(TUBE_TEXTURE)
				entry.concat(' 0 0 0 1 1 0 32 0')
				file.puts entry
				entry = '' #left face
				entry.concat(vertex_string(segment_outer[j+1][i]))
				entry.concat(vertex_string(segment_outer[j][i]))
				entry.concat(vertex_string(segment_inner[j][i]))
				entry.concat(TUBE_TEXTURE)
				entry.concat(' 0 0 0 1 1 0 32 0')
				file.puts entry
				entry = '' #right face
				entry.concat(vertex_string(segment_outer[j][i+1]))
				entry.concat(vertex_string(segment_outer[j+1][i+1]))
				entry.concat(vertex_string(segment_inner[j][i+1]))
				entry.concat(TUBE_TEXTURE)
				entry.concat(' 0 0 0 1 1 0 32 0') # 32 = trans66, 16= trans33
				file.puts entry
				file.puts '}'
			end
			i = 0
		end

		file.puts '}'
		file.close
		puts 'Wrote prefab file ' + FILE_TUBE
		
		file = open(FILE_LIQUID, 'w')
		file.puts '{'
		file.puts '"classname" "worldspawn"'
    
		for j in 0..segments-1

			file.puts "// brush #{j}"
			file.puts '{'
			
			# Calculate how much to rotate textures as view down the z-axis
		
			for i in 0..sides-1
				nx, ny, nz = GetNormal(segment_inner[j][i], segment_inner[j+1][i], segment_inner[j+1][i+1]) #' <-- Notepad++ gets syntax highlighting wrong here, need this comment to fix it :)
#				puts 'Normal for segment %d, face %d: x=%.20f, y=%.20f, z=%f' % [j, i, nx, ny, nz]
				inward_facing = IsInward(segment_inner[j][i], [nx, ny, nz])
				px, py, pz = GetProjection([nx, ny, nz])
#				puts '                              x=%d, y=%d, z=%d' % [nx, ny, nz] # facing axis

#				if inward_facing
#					puts 'Segment %d, face %d faces inward'%[j, i]
#				else
#					puts 'Segment %d, face %d faces outward'%[j, i]
#				end
				
				if (px > 0)
					if inward_facing
						texture_rotation = 0
					else
						texture_rotation = 180
					end
				elsif (px < 0)
					if inward_facing
						texture_rotation = 180
					else
						texture_rotation = 0
					end
				elsif (py > 0)
					if inward_facing
						texture_rotation = 180
					else
						texture_rotation = 0
					end
				elsif (py < 0)
					if inward_facing
						texture_rotation = 0
					else
						texture_rotation = 180
					end
				elsif(pz.abs > 0)
					texture_rotation = (360.0 - ((j+0.5) * BEND_ANGLE / BEND_SEGMENTS))
				else
					puts 'Eh? No surface normal found.'
				end
				
				entry = ''
				entry.concat(vertex_string(segment_inner[j][i]))
				entry.concat(vertex_string(segment_inner[j+1][i]))
				entry.concat(vertex_string(segment_inner[j+1][i+1]))
				entry.concat(LIQUID_TEXTURE)
				entry.concat(' 0 0');
				entry.concat(' %.2f' % [texture_rotation])
				if LIQUID_FLOWING
					entry.concat(' 0.25 0.25 0 64 0') #'
				else
					entry.concat(' 0.25 0.25 0 0 0') #'	
				end
#				entry.concat(i.to_s) # value
				file.puts entry
			end
			
						
			i = 0
			
			# now add planes that define the ends of the surfaces
			entry = ''
			entry.concat(FORMAT_STRING % [ segment_inner[j][i][0], segment_inner[j][i][1], segment_inner[j][i][2] ])
			entry.concat(FORMAT_STRING % [ segment_inner[j][i+1][0], segment_inner[j][i+1][1], segment_inner[j][i+1][2] ])
			entry.concat(FORMAT_STRING % [ segment_inner[j][i+2][0], segment_inner[j][i+2][1], segment_inner[j][i+2][2] ])
			entry.concat(LIQUID_TEXTURE)
			entry.concat(' 0 0 0 1 1 0 64 0')
			file.puts entry

			entry = ''
			entry.concat(FORMAT_STRING % [ segment_inner[j+1][i+1][0], segment_inner[j+1][i+1][1], segment_inner[j+1][i+1][2] ])
			entry.concat(FORMAT_STRING % [ segment_inner[j+1][i][0], segment_inner[j+1][i][1], segment_inner[j+1][i][2] ])
			entry.concat(FORMAT_STRING % [ segment_inner[j+1][i+2][0], segment_inner[j+1][i+2][1], segment_inner[j+1][i+2][2] ])
			entry.concat(LIQUID_TEXTURE)
			entry.concat(' 0 0 0 1 1 0 64 0')
			file.puts entry
			
			file.puts '}'
		end

		file.puts '}'
		file.close
		puts 'Wrote prefab file '+ FILE_LIQUID
		
		puts 'Controls:'
		puts '      Arrow keys - translate x,y'
		puts '      Page up/dn - zoom in/out'
		puts '      Mouse      - rotate'
		puts '      Esc        - exit'
	end

	def vertex_string(a) # takes a vertex, produces a formatted string
		return FORMAT_STRING % [ a[0], a[1], a[2] ]
	end
	
	# takes three sets of vertices, and returns the surface normal
	def GetNormal(a, b, c) 
		u = Array.new
		v = Array.new
		n = Array.new
		
		# calculate difference vectors
		for i in 0..2
			u[i] = a[i] - b[i]
			v[i] = c[i] - b[i]
		end
		
		# calculate cross product
		n[0] = u[1]*v[2] - u[2]*v[1]
		n[1] = u[2]*v[0] - u[0]*v[2]
		n[2] = u[0]*v[1] - u[1]*v[0]
		
		# calculate length of cross product vector
		l = Math.sqrt(n[0]**2 + n[1]**2 + n[2]**2)
		
		# normalise cross product vector
		for i in 0..2
			n[i] = n[i] / l
		end
		
		return n
	end
	
	# determines from a face normal and a vertex whether the face is looking inward or outward from the curve
	def IsInward(vec1, n)
		vec2 = Array.new
		for i in 0..2
			vec2[i] = vec1[i] + n[i]
		end
		
		# calc vector lengths from origin (center point of bend)
		l1 = Math.sqrt(vec1[0]**2 + vec1[1]**2 + vec1[2]**2)
		l2 = Math.sqrt(vec2[0]**2 + vec2[1]**2 + vec2[2]**2)
	
		return l1 > l2
	end
	
	# takes surface normal and calculates which axis the game engine projects the texture down
	def GetProjection(n) 
	
		# game doesn't seem to use same precision as Ruby, so must apply some rounding
		# this is just a guess at the level of rounding needed however
		n[0] = ((n[0]*10000000).round)/10000000.0
		n[1] = ((n[1]*10000000).round)/10000000.0
		n[2] = ((n[2]*10000000).round)/10000000.0

		if n[0].abs  >= n[1].abs  # + 0.000000000000001 ?
			if n[0].abs >= n[2].abs
				# Biggest is n[0]
				n[0] = n[0] >= 0 ? 1 : -1
				n[1] = 0;
				n[2] = 0;
			else
				# Biggest is n[2]
				n[0] = 0;
				n[1] = 0;
				n[2] = n[2] >= 0 ? 1 : -1
			end
		elsif n[1].abs >= n[2].abs # n[1] >= n[0]
			# Biggest is n[1] 
			n[0] = 0;
			n[1] = n[1] >= 0 ? 1 : -1
			n[2] = 0;
		else
			#biggest is n[2]
			n[0] = 0;
			n[1] = 0;
			n[2] = n[2] >= 0 ? 1 : -1
		end
		
		return n
	end
	
  def draw
    GL.Clear(GL::COLOR_BUFFER_BIT | GL::DEPTH_BUFFER_BIT);

	GL.PushMatrix()
    GL.Translate(@view_transx, 0.0, 0.0)
    GL.Translate(0.0, @view_transy, 0.0)
    GL.Translate(0.0, 0.0, @view_transz)
    GL.PushMatrix()
	
    GL.PushMatrix()
    GL.Rotate(@view_rotx, 1.0, 0.0, 0.0)
    GL.Rotate(@view_roty, 0.0, 1.0, 0.0)
    GL.Rotate(@view_rotz, 0.0, 0.0, 1.0)

    GL.PushMatrix()
#    GL.Translate(-3.0, -2.0, 0.0)
    GL.Rotate(@angle, 0.0, 1.0, 0.0)
    GL.CallList(@tube)
    GL.PopMatrix()

    GL.PopMatrix()

    GLUT.SwapBuffers()
  end

  def idle
    t = GLUT.Get(GLUT::ELAPSED_TIME) / 1000.0
    @t0_idle = t if !defined? @t0_idle
    # 90 degrees per second
	if @autorotate == 1
		@angle += 70.0 * (t - @t0_idle)
	end
    @t0_idle = t
    GLUT.PostRedisplay()
	# This is cludgy and nasty, but GLUT.CloseFunc() callback appears to not be supported in Ruby yet - no choice but to use this to determine if the window was closed :/
	if GLUT.Get(GLUT::WINDOW_X) == 0 and GLUT.Get(GLUT::WINDOW_Y) == 0
		exit
	end
  end

  # Change view angle, exit upon ESC
  def key(k, x, y)
    case k
      when ?z
        @view_rotz += 5.0
      when ?Z
        @view_rotz -= 5.0
      when 27 # Escape
        exit
    end
    GLUT.PostRedisplay()
  end

  # Change view angle
  def special(k, x, y)
    case k
      when GLUT::KEY_UP
        @view_transy -= 10.0
      when GLUT::KEY_DOWN
        @view_transy += 10.0
      when GLUT::KEY_LEFT
        @view_transx += 10.0
      when GLUT::KEY_RIGHT
        @view_transx -= 10.0
      when GLUT::KEY_PAGE_UP
        @view_transz += 10.0
      when GLUT::KEY_PAGE_DOWN
        @view_transz -= 10.0
    end
    GLUT.PostRedisplay()
  end

  # New window size or exposure
  def reshape(width, height)
    h = height.to_f / width.to_f
    GL.Viewport(0, 0, width, height)
    GL.MatrixMode(GL::PROJECTION)
    GL.LoadIdentity()
	#Definition: Frustum(left, right, bottom, top, zNear, zFar)
    GL.Frustum(-1.0, 1.00, -h, h, 5.0, 2000.0)
    GL.MatrixMode(GL::MODELVIEW)
    GL.LoadIdentity()
    GL.Translate(0.0, 0.0, -500.0)
  end

  def init
    @angle = 0.0
    @view_rotx, @view_roty, @view_rotz = 20.0, 30.0, 0.0
	@view_transx, @view_transy, @view_transz = 0.0, 0.0, 0.0
	@autorotate = 1
	
    GL.Lightfv(GL::LIGHT0, GL::POSITION, POS)
    GL.Enable(GL::CULL_FACE)
    GL.Enable(GL::LIGHTING)
    GL.Enable(GL::LIGHT0)
    GL.Enable(GL::DEPTH_TEST)

    @tube = GL.GenLists(1)
    GL.NewList(@tube, GL::COMPILE)
    GL.Material(GL::FRONT, GL::AMBIENT_AND_DIFFUSE, RED)
    tube(TUBE_RADIUS, BEND_RADIUS, TUBE_SIDES, BEND_SEGMENTS)
    GL.EndList()

    GL.Enable(GL::NORMALIZE)

    ARGV.each do |arg|
      case arg
        when '-info'
          printf("GL_RENDERER   = %s\n", GL.GetString(GL::RENDERER))
          printf("GL_VERSION    = %s\n", GL.GetString(GL::VERSION))
          printf("GL_VENDOR     = %s\n", GL.GetString(GL::VENDOR))
          printf("GL_EXTENSIONS = %s\n", GL.GetString(GL::EXTENSIONS))
      end
    end
  end

  def visible(vis)
    GLUT.IdleFunc((vis == GLUT::VISIBLE ? method(:idle).to_proc : nil))
  end

  def mouse(button, state, x, y)
    @mouse = state
    @x0, @y0 = x, y
  end
  
  def motion(x, y)
    if @mouse == GLUT::DOWN then
		@autorotate = 0
		@view_roty += @x0 - x
		@view_rotx += @y0 - y
    end
    @x0, @y0 = x, y
  end

  def initialize
    GLUT.Init()
    GLUT.InitDisplayMode(GLUT::RGB | GLUT::DEPTH | GLUT::DOUBLE)

    GLUT.InitWindowPosition(300, 300)
    GLUT.InitWindowSize(200, 200)
    GLUT.CreateWindow('Tube generator')
	
    init()
	
    GLUT.DisplayFunc(method(:draw).to_proc)
    GLUT.ReshapeFunc(method(:reshape).to_proc)
    GLUT.KeyboardFunc(method(:key).to_proc)
    GLUT.SpecialFunc(method(:special).to_proc)
    GLUT.VisibilityFunc(method(:visible).to_proc)
    GLUT.MouseFunc(method(:mouse).to_proc)
    GLUT.MotionFunc(method(:motion).to_proc)
#	GLUT.CloseFunc(method(:close).to_proc)  # Ruby bindings don't define this yet :(
  end

  def start
    GLUT.MainLoop()
  end
  
end

Tube.new.start

