#!/usr/bin/env ruby

=begin

    RUBY PREFAB GENERATOR
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

TUBE_RADIUS  = 16.0
TORUS_RADIUS = 64.0
TUBE_SIDES = 16
TORUS_SEGMENTS = 16
OUTPUTFILE = 'torus.pfb'

class Tube

  POS = [5.0, 5.0, 10.0, 0.0]
  RED = [0.8, 0.1, 0.0, 1.0]
  GREEN = [0.0, 0.8, 0.2, 1.0]
  BLUE = [0.2, 0.2, 1.0, 1.0]

  include Math

  # from http://mathworld.wolfram.com/Torus.html
  def calc_tube_segment(tube_radius, torus_radius, sides, segment_angle)
  	ring = Array.new
    for i in 0..sides
		vertex = Array.new
		tube_angle = i * 2.0 * PI / sides
		
		vertex[0] = (torus_radius + tube_radius*cos(tube_angle))*cos(segment_angle);		# x
		vertex[1] = (torus_radius + tube_radius*cos(tube_angle))*sin(segment_angle);		# y
		vertex[2] = tube_radius*sin(tube_angle);											# z		
		
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
	def tube(tube_radius, torus_radius, sides, segments)
		GL.ShadeModel(GL::SMOOTH)
		#GL.ShadeModel(GL::FLAT)
		#GL.Normal(0.0, 0.0, 1.0)

		segment = Array.new
		for i in 0..segments
			segment << calc_tube_segment(tube_radius, torus_radius, sides, i * 2.0 * PI / segments)
			#segment << calc_straight_segment(tube_radius, sides, i * 50)
		end

		GL.Normal(0.0, 0.0, 1.0)
		for j in 0..segments-1
			seg_angle=j * 2.0 * PI / segments
			GL.Begin(GL::QUADS)
			for i in 0..sides-1
				face_angle = i * 2.0 * PI / sides
#				GL.Normal(cos(face_angle), sin(face_angle), 0)
				GL.Normal(cos(face_angle)*cos(seg_angle), cos(face_angle)*sin(seg_angle), sin(face_angle));			
				GL.Vertex3f(segment[j][i][0], 		segment[j][i][1], 		segment[j][i][2]);
				GL.Vertex3f(segment[j+1][i][0], 	segment[j+1][i][1], 	segment[j+1][i][2]);
				GL.Vertex3f(segment[j+1][i+1][0], 	segment[j+1][i+1][1],	segment[j+1][i+1][2]);
				GL.Vertex3f(segment[j][i+1][0], 	segment[j][i+1][1], 	segment[j][i+1][2]);
			end
			GL.End()
		end

		file = open(OUTPUTFILE, 'w')
		file.puts '{'
		file.puts '"classname" "worldspawn"'

		for j in 0..segments-1

			file.puts "// brush #{j}"
			file.puts '{'
		
			for i in 0..sides-1
				entry = ''
				entry.concat("( %.6f %.6f %.6f ) " % [ segment[j+1][i][0], segment[j+1][i][1], segment[j+1][i][2] ])
				entry.concat("( %.6f %.6f %.6f ) " % [ segment[j][i][0], segment[j][i][1], segment[j][i][2] ])
				entry.concat("( %.6f %.6f %.6f ) " % [ segment[j+1][i+1][0], segment[j+1][i+1][1], segment[j+1][i+1][2] ])
				entry.concat('no_texture 0 0 0 1 1 0 0 0')
				file.puts entry
			end
			i = 0
			
			# now add planes that define the ends of the surfaces
			entry = ''
			entry.concat("( %.6f %.6f %.6f ) " % [ segment[j][i+1][0], segment[j][i+1][1], segment[j][i+1][2] ])
			entry.concat("( %.6f %.6f %.6f ) " % [ segment[j][i][0], segment[j][i][1], segment[j][i][2] ])
			entry.concat("( %.6f %.6f %.6f ) " % [ segment[j][i+2][0], segment[j][i+2][1], segment[j][i+2][2] ])
			entry.concat('no_texture 0 0 0 1 1 0 0 0')
			file.puts entry

			entry = ''
			entry.concat("( %.6f %.6f %.6f ) " % [ segment[j+1][i][0], segment[j+1][i][1], segment[j+1][i][2] ])
			entry.concat("( %.6f %.6f %.6f ) " % [ segment[j+1][i+1][0], segment[j+1][i+1][1], segment[j+1][i+1][2] ])
			entry.concat("( %.6f %.6f %.6f ) " % [ segment[j+1][i+2][0], segment[j+1][i+2][1], segment[j+1][i+2][2] ])
			entry.concat('no_texture 0 0 0 1 1 0 0 0')
			file.puts entry
			
			file.puts '}'
		end

		file.puts '}'
		file.close
		puts 'Wrote prefab file '+OUTPUTFILE
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
=begin
      when GLUT::KEY_UP
        @view_rotx += 5.0
      when GLUT::KEY_DOWN
        @view_rotx -= 5.0
      when GLUT::KEY_LEFT
        @view_roty += 5.0
      when GLUT::KEY_RIGHT
        @view_roty -= 5.0
=end
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
    GL.Frustum(-1.0, 1.00, -h, h, 5.0, 600.0)
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
    tube(TUBE_RADIUS, TORUS_RADIUS, TUBE_SIDES, TORUS_SEGMENTS)
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

