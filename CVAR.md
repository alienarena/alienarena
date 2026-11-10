# CVARs Documentation

## CLIENT SIDE CVARs

### Input/Control CVARs

**in_mouse**: BOOL (default: 1)

Enable/disable mouse input processing.

**in_dgamouse**: BOOL (default: 1)

Use DGA mouse on Linux for raw input (improved precision).

**m_direct**: BOOL (default: 0)

Direct mouse delta mode. 0=delta from center, 1=delta from last position (lower latency).

**m_accel**: BOOL (default: 1)

Enable mouse acceleration.

**sensitivity**: FLOAT (default: 3)

Mouse sensitivity multiplier for aiming.

**menu_sensitivity**: FLOAT (default: 3)

Mouse sensitivity in menu interface.

**m_pitch**: FLOAT (default: 0.022)

Mouse pitch (vertical look) sensitivity.

**m_yaw**: FLOAT (default: 0.022)

Mouse yaw (horizontal look) sensitivity.

**m_forward**: FLOAT (default: 1)

Mouse forward/backward movement multiplier.

**m_side**: FLOAT (default: 1)

Mouse strafe (left/right) movement multiplier.

**in_joystick**: BOOL (default: 0)

Enable joystick/gamepad input.

**joy_forwardthreshold**: FLOAT (default: 0.15)

Joystick forward/back dead zone threshold.

**joy_sidethreshold**: FLOAT (default: 0.15)

Joystick strafe dead zone threshold.

**joy_upthreshold**: FLOAT (default: 0.15)

Joystick vertical movement dead zone threshold.


### Console/Terminal CVARs

**con_notifytime**: INT (default: 6)

Delay in seconds before the console will show another notification; used to throttle rapid message spam.

**con_ignorecolorcodes**: BOOL (default: 0)

When set, color escape codes embedded in console text are stripped, which can make log files easier to read.

**con_color**: INT (default: 0)

Selects the colour used for the console background. 0 = off (black), 1 = red, 2 = green, 3 = blue, 4 = cyan, 5 = purple

### System/Developer CVARs

**nostdout**: BOOL (default: 0)

Do not echo engine output to standard output.  Useful when running a dedicated server in the background.

**sys_ansicolor**: BOOL (default: 1)

Enable conversion of ANSI colour codes to the internal colour format on Unix terminals.  If disabled the codes are left intact.


**paused**: BOOL (default: 0)

Set while the game is paused.

**p100**: INT (default: 2, read-only)

Internal userinfo value sent to the server (used for compatibility with the server).

**basedir**: STRING (default: ".", read-only)

Base directory of the game data.

### Movement CVARs

**cl_upspeed**: INT (default: 200)

Up/down movement speed.

**cl_forwardspeed**: INT (default: 200)

Forward movement speed.

**cl_sidespeed**: INT (default: 200)

Strafe (left/right) movement speed.

**cl_yawspeed**: INT (default: 140)

Rotation speed when using keyboard.

**cl_pitchspeed**: INT (default: 150)

Look up/down speed when using keyboard.

**cl_anglespeedkey**: FLOAT (default: 1.5)

Multiplier for angle speed when holding speed key.

**cl_run**: BOOL (default: 0)

Always run (toggle behavior).

**freelook**: BOOL (default: 0)

Enable free look mode without aiming.

**lookspring**: BOOL (default: 0)

Auto-center view when releasing look keys.

**lookstrafe**: BOOL (default: 0)

Use mouse for strafing instead of looking.

### Display/Graphics CVARs

**cl_maxfps**: INT (default: 100)

Limit frames drawn per second using a busy-wait loop. Higher values = more responsive input.

**cl_maxpackets**: INT (default: 100)

Maximum number of packets to send to the server per second. Limited by the server tickrate (`sv_tickrate`).

**cl_drawfps**: BOOL (default: 0)

Draw FPS counter on screen.

**cl_blend**: BOOL (default: 1)

Enable color blending effects (pain flash, powerup effects).

**cl_lights**: BOOL (default: 1)

Render dynamic lighting (CPU intensive).

**cl_particles**: BOOL (default: 1)

Render particle effects (explosions, tracers).

**cl_entities**: BOOL (default: 1)

Render entity models (players, items, monsters).

**cl_gun**: BOOL (default: 1)

Render first-person weapon model.

**cl_flicker**: BOOL (default: 1)

Enable flickering world lighting.

**cl_simpleitems**: BOOL (default: 0)

Show sprites instead of models for pickup items.

**cl_centerprint**: BOOL (default: 1)

Show center-screen messages (kills, pickups).

**cl_vwep**: BOOL (default: 1)

Show weapon models held by other players.

**cl_noblood**: BOOL (default: 0)

Disable blood effects for gore-sensitive players.

**cl_brass**: BOOL (default: 1)

Render shell casings from weapons.

**cl_noskins**: BOOL (default: 0)

Use default model instead of custom player skins.

### Network/Rate Control CVARs

**rate**: INT (default: 40000)

Bandwidth limit in bytes/second (userinfo cvar). The server clamps it to the range 2500 to 75000.

**entity_cull**: FLOAT (default: 0)

Per-client entity culling override (userinfo cvar): cull entities smaller than this angular size (in degrees). Higher values = cull more aggressively. 0 uses the server default (`sv_entity_cull`).

**cl_nodelta**: BOOL (default: 0)

Ask the server for full (non-delta) updates only. Debugging only, uses a lot of bandwidth.

**cl_timeout**: INT (default: 120)

Disconnect if no server response for this many seconds.

**cl_shownet**: INT (default: 0)

Show network traffic debug info. Higher values = more detail.

**cl_showmiss**: BOOL (default: 0)

Print a message when the client prediction does not match the server.

**showclamp**: BOOL (default: 0)

Print a message when the client time is clamped.

**netgraph**: BOOL (default: 0)

Draw the network graph on screen (see also `graphheight`, `graphscale`, `graphshift`, `debuggraph`, `timegraph`).

**cl_drawbandwidth**: BOOL (default: 0)

Show the incoming bandwidth from the server on screen, in KiB/s.

**cl_drawframesover**: INT (default: 0)

Show on screen the number of frames per second that took longer than this many milliseconds (a measure of frame time jitter). 0 = off.

**cl_drawtimer**: BOOL (default: 0)

Show a timer on screen.

**net_shownet**: INT (default: 0)

(Windows) Show network debug output.

**noudp**, **noipx**: BOOL (default: 0)

Disable the UDP or IPX network protocol (set on the command line).

**ip**: STRING (default: "localhost")

IP address of the server to use (read-only, set on the command line).

**port**: INT (default: 27910)

UDP port of the server (read-only, set on the command line).

**clientport**, **hostport**, **ip_clientport**, **ip_hostport**, **ipx_clientport**, **ipx_hostport**: INT (default: 0 for the host and IP ports)

(Windows) Ports used for network sockets (read-only, set on the command line).

**qport**: INT (default: random)

Random port number that identifies the client to the server independent of its NAT address (read-only).

**showdrop**: BOOL (default: 0)

Print a message when packets are dropped.

**showpackets**: BOOL (default: 0)

Print all sent and received packets.

### User Info CVARs

**name**: STRING (default: "unnamed")

Player name (sent to server, case-sensitive when choosing servers).

**skin**: STRING (default: "male/grunt")

Character model and skin selection.

**hand**: INT (default: 0)

Weapon hand: 0=gun shown on the right, 1=gun shown on the left, 2=gun hidden.

**gender**: STRING (default: "male")

Player gender for announcements ("male", "female", "neutral").

**fov**: INT (default: 90)

Field of view in degrees (max 160). Higher = wider view but more distortion.

**password**: STRING (default: "")

Password for joining password-protected servers.

**spectator**: BOOL (default: 0)

Enable spectator mode.

**spectator_password**: STRING (default: "")

Password for spectator mode on servers requiring it.

**msg**: INT (default: 1)

Message filter level for server broadcasts.

### Game Client CVARs

**cl_predict**: BOOL (default: 1)

Predict own movement locally for lower perceived latency.

**cl_autoskins**: BOOL (default: 0)

Automatically download player skins when joining servers.

**cl_precachecustom**: BOOL (default: 0)

Precache 3rd-party and custom player skins at the first map load (uses more memory/time).

**cl_playtaunts**: BOOL (default: 1)

Play taunt sounds when players use them.

**cl_footsteps**: BOOL (default: 1)

Play footstep sounds when players move.

**cl_test**: BOOL (default: 0)

Enable test features/debug mode.

**timedemo**: BOOL (default: 0)

Play back demos (recorded games) in benchmark mode: runs the demo at maximum speed and prints timing stats.

**demoquit**: BOOL (default: 0)

Quit automatically after a demo has finished.

**cl_showplayernames**: INT (default: 0)

Player nametags: 0=none, 1=one nametag in the center of the screen, 2=a nametag over each player.

**cl_paindist**: BOOL (default: 0)

Distort the screen when you take damage.

**cl_explosiondist**: BOOL (default: 0)

Distort the screen for explosions.

**cl_raindist**: BOOL (default: 1)

Show rain running down your visor.

**cl_dm_lights**: BOOL (default: 0)

Enable Deathmatch-specific dynamic lighting optimizations.

**cl_disbeamclr**: INT (default: 0)

Beam color of the disruptor weapon: 0=green, 1=blue, 2=red, 3=yellow, 4=purple.

**cl_showspeedometer**: BOOL (default: 0)

Display real-time player speed (movement units/sec).

**cl_showaccelerometer**: BOOL (default: 0)

Display acceleration magnitude in real-time.

### Server Interaction CVARs

**rcon_password**: STRING (default: "")

Password for remote console (rcon) commands on servers.

**rcon_address**: STRING (default: "")

IP address for sending rcon commands.

**cl_master**: STRING (default: "master.alienarena.org")

Primary master server for getting server list.

**cl_master2**: STRING (default: "master2.alienarena.org")

Secondary master server for redundancy.

**cl_show_active_servers_only**: BOOL (default: 0)

Only show active servers in the server list. Not remembered when quitting.

**stats_password**: STRING (default: "password")

Password for your player stats account on the stats server (stored in your profile).

**stats_pw_hashed**: BOOL (default: 0)

Set automatically when `stats_password` has been stored as a hash.

**cl_stats_server**: STRING (default: "http://martianbackup.com")

Stats collection server URL for player rankings.

**cl_latest_game_version_server**: STRING (default: "https://invader.alienarena.org/version/crx_version")

URL for checking the latest game version.

**cl_speedrecord**: INT (default: 450)

Personal speed run record (set by player).

**cl_alltimespeedrecord**: INT (default: 450)

All-time speed run record (persistent).

### Audio CVARs

**background_music**: BOOL (default: 1)

Enable background music in menus and during gameplay.

**background_music_vol**: FLOAT (default: 0.5)

Background music volume level (0.0 to 1.0).

### HUD/Display Image CVARs

**cl_hudimage1**: STRING (default: "pics/i_health.tga")

Custom HUD image 1 (health display).

**cl_hudimage2**: STRING (default: "pics/i_score.tga")

Custom HUD image 2 (score display).

**cl_hudimage3**: STRING (default: "pics/i_ammo.tga")

Custom HUD image 3 (ammo display).

### Font CVARs

**fnt_game**: STRING (default: "orbitron")

Font face for in-game text.

**fnt_game_size**: INT (default: 0)

Game font size (0 = auto).

**fnt_console**: STRING (default: "freemono")

Font for console text.

**fnt_console_size**: INT (default: 0)

Console font size (0 = auto).

**fnt_menu**: STRING (default: "freesans")

Font for menu text.

**fnt_menu_size**: INT (default: 0)

Menu font size (0 = auto).

### Chat/IRC CVARs

**cl_chatlines**: INT (default: 10)

Number of chat messages shown in the chat overlay.

**cl_IRC_server**: STRING (default: "irc.alienarena.org")

IRC server to connect to.

**cl_IRC_port**: INT (default: 6667)

IRC server port.

**cl_IRC_channel**: STRING (default: "alienarena")

IRC channel to join.

**cl_IRC_nickname**: STRING (default: "")

IRC nickname. Empty = use your player name.

**cl_IRC_override_nickname**: BOOL (default: 0)

Use `cl_IRC_nickname` instead of the player name.

**cl_IRC_connect_at_startup**: BOOL (default: 1)

Connect to IRC when the game starts.

**cl_IRC_kick_rejoin**: BOOL (default: 0)

Rejoin the channel automatically after being kicked.

**cl_IRC_reconnect_delay**: INT (default: 100)

Delay before reconnecting after an IRC disconnect (minimum 5).

### Screen/HUD CVARs

**crosshair**: INT (default: 0)

Crosshair selection (0 = none).

**scr_centertime**: FLOAT (default: 2.5)

Seconds that center-screen messages stay visible.

**scr_consize**: FLOAT (default: 0.5)

Fraction of the screen covered by the console.

**scr_conspeed**: FLOAT (default: 3)

Speed at which the console slides down and up.

**scr_printspeed**: FLOAT (default: 8)

Speed at which center-screen text is printed.

**scr_showpause**: BOOL (default: 0), **scr_showturtle**: BOOL (default: 0)

Show the pause and the "turtle" (slow frame) indicators.

**cl_stereo**: BOOL (default: 0), **cl_stereo_separation**: FLOAT (default: 0.4)

Stereo rendering and the separation between the two views.

**fnt_center_size**: INT (default: 0)

Font size of center-screen messages (0 = auto).

**gender_auto**: BOOL (default: 1)

Automatically set `gender` from the selected player model.

**adr0** to **adr8**: STRING (default: "")

Address book entries of the server browser.

**con_font**: STRING (default: "default")

Font used for the console.

### Sound CVARs

**s_volume**: FLOAT (default: 0.1)

Sound effects volume.

**s_device**: STRING (default: "Default")

OpenAL sound device to use.

**s_doppler**: FLOAT (default: 0.0)

Doppler effect factor (0 = off).

**s_initsound**: BOOL (default: 1)

Initialise the sound system at startup.

**s_minsources**, **s_maxsources**: INT

Lower and upper limits on the number of OpenAL sound sources generated (maximum 128).

**snd_developer**: BOOL (default: 0)

Print extra sound system information.

### Video/Renderer CVARs

**vid_ref**: STRING (default: "gl", read-only)

Video renderer.

**vid_fullscreen**: INT (default: 1)

Fullscreen mode.

**vid_preferred_fullscreen**: INT (default: 1)

Preferred fullscreen mode.

**vid_width**, **vid_height**: INT (default: 1024, 768)

Custom resolution, used when `gl_mode` is -1.

**gl_mode**: INT (default: 3)

Video mode (see `vid_modelist`).

**vid_xpos**, **vid_ypos**: INT (default: 0)

Window position.

**vid_gamma**: FLOAT (default: 1.0), **vid_contrast**: FLOAT (default: 1.0)

Gamma and contrast of the display.

**vid_displayfrequency**: INT (default: 0), **win_noalttab**: BOOL (default: 0)

(Windows) Display refresh rate and disabling of Alt-Tab.

**r_fakeFullscreen**: BOOL (default: 0)

(Linux) Use a borderless fullscreen window instead of exclusive fullscreen.

**r_firstrun**: BOOL (default: 0)

Set this to 0 if you want the game to auto detect your graphics settings next time you run it.

**gl_swapinterval**: BOOL (default: 1)

Sync to Vblank (0=off, 1=on). Keep enabled for smooth display and power efficiency. Disable only for competitive ultra-low-latency play (will cause screen tearing).

**gl_modulate**: FLOAT (default: 2)

Brightness setting. Higher means brighter.

**gl_picmip**: INT (default: 0)

Texture detail. 0 means full detail. Each higher setting has 1/4 less detail.

**gl_texturemode**: STRING (default: "GL_LINEAR_MIPMAP_LINEAR")

Texture filtering mode. See also `r_anisotropic`, `r_ext_max_anisotropy` and `r_alphamasked_anisotropic`.

**r_antialiasing**: INT (default: 0)

Multisample antialiasing samples (needs a video restart).

**gl_finish**: BOOL (default: 0)

Wait for the graphics driver to finish drawing each frame before drawing the next one. Hurts performance but may improve smoothness on very low-end machines.

**gl_cull**: BOOL (default: 1)

Avoid rendering anything that's off the edge of the screen. Good for performance, recommend leaving it on.

**gl_fog**: BOOL (default: 1)

Fog and weather effects.

**gl_dynamic**: BOOL (default: 1)

Dynamic lighting.

**gl_vlights**: BOOL (default: 1)

Vertex lighting on models.

**gl_mirror**: BOOL (default: 1)

Reflective surfaces.

**gl_shadowmaps**: BOOL (default: 0), **r_shadowcutoff**: FLOAT (default: 880), **r_shadowmapscale**: FLOAT (default: 1)

Shadow maps, the distance beyond which shadows are not drawn, and the shadow map resolution scale.

**r_shaders**: BOOL (default: 1), **r_meshnormalmaps**: BOOL (default: 1), **r_worldnormalmaps**: BOOL (default: 0)

Shader effects and normal mapping on models and on the world.

**r_lightmapfiles**: BOOL (default: 1)

Enables the loading of .lightmap files, with more detailed light and shadow. Turn this off if video RAM is limited.

**r_optimize**: BOOL (default: 1)

Skip BSP recursion unless you move. Good for performance, recommend leaving it on.

**r_overbrightbits**: INT (default: 2)

Overbright lighting.

**r_bloom**: BOOL (default: 1)

Bloom effect. Fine tuning: `r_bloom_alpha` (0.2), `r_bloom_intensity` (0.75), `r_bloom_darken` (8), `r_bloom_sample_scaledown` (2), `r_bloom_diamond_size` (8), `r_bloom_autoexposure` (1).

**r_godrays**: BOOL (default: 0), **r_godray_intensity**: FLOAT (default: 1.0)

Light shafts from the sun and their intensity.

**r_drawsun**: INT (default: 2)

Sun rendering (0 = off).

**r_lensflare**: BOOL (default: 1), **r_lensflare_intens**: INT (default: 3), **r_lightbeam**: BOOL (default: 1)

Lens flares, their intensity, and light beams.

**r_vegetation**: BOOL (default: 1)

Draw grass and other vegetation.

**r_ragdolls**: BOOL (default: 1)

Ragdoll physics for dead players.

**r_minimap**: BOOL (default: 0), **r_minimap_size**: INT (default: 256), **r_minimap_style**: INT (default: 1), **r_minimap_zoom**: FLOAT (default: 1)

On-screen minimap and its size, style and zoom.

**gl_screenshot_type**: STRING (default: "jpeg"), **gl_screenshot_jpeg_quality**: INT (default: 85)

Screenshot format and JPEG quality.

**ttf_autohint**: BOOL (default: 0), **ttf_subpixel**: BOOL (default: 0)

TrueType font rendering options.

**sys_priority**: BOOL (default: 0)

(Windows) Raise the process priority while running in exclusive fullscreen.

**sys_affinity**: INT (default: 1)

(Windows only, ignored elsewhere) CPU core(s) the game process is restricted to, as a bitmask: 0 = don't set affinity, 1 = core 0 only, 2 = core 1 only, 3 = cores 0 and 1 (two cores/threads instead of one). Values above 3 are clamped to 3. Only applied on machines with more than one core.

### Joystick CVARs

**in_initmouse**, **in_initjoy**: BOOL (default: 1)

(Windows) Initialise the mouse and the joystick at startup.

**joy_name**: STRING (default: "joystick")

Joystick name.

**joy_advanced**: BOOL (default: 0)

Use the advanced joystick axis mapping (`joy_advaxisx`, `joy_advaxisy`, `joy_advaxisz`, `joy_advaxisr`, `joy_advaxisu`, `joy_advaxisv`).

**joy_forwardsensitivity**, **joy_sidesensitivity**, **joy_upsensitivity**, **joy_yawsensitivity**: FLOAT (default: -1), **joy_pitchsensitivity**: FLOAT (default: 1)

Joystick axis sensitivities.

**joy_pitchthreshold**, **joy_yawthreshold**: FLOAT (default: 0.15)

Joystick pitch and yaw dead zones.

**v_centermove**: FLOAT (default: 0.15), **v_centerspeed**: FLOAT (default: 500)

Look-spring tuning.

### Debug CVARs (client)

**cl_stats**, **cl_testblend**, **cl_testentities**, **cl_testlights**, **cl_testparticles**: BOOL (default: 0)

Test and statistics output for the client view.

**debuggraph**, **timegraph**: BOOL (default: 0), **graphheight**: INT (default: 32), **graphscale**: FLOAT (default: 1), **graphshift**: FLOAT (default: 0)

Draw a debug or timing graph on screen.

**rspeed_beams**, **rspeed_epolys**, **rspeed_flares**, **rspeed_grasses**, **rspeed_particles**, **rspeed_vbobatches**, **rspeed_wpolys**: BOOL (default: 0)

Show renderer counters on screen.

**scr_timerefcount**: INT (default: 128)

Number of frames of the `timerefresh` benchmark.

**gl_lightmap**, **gl_lockpvs**, **gl_nobind**, **gl_nosubimage**, **gl_polyblend**, **gl_showtris**, **gl_skymip**, **gl_clear**, **gl_bitdepth**, **gl_drawbuffer**, **gl_driver**, **gl_texturealphamode**, **gl_texturesolidmode**, **r_drawentities**, **r_drawworld**, **r_fullbright**, **r_lerpmodels**, **r_nocull**, **r_norefresh**, **r_novis**, **r_test**, **flushmap**, **scriptsloaded**

Renderer debugging and test cvars; leave them at their defaults.

**gl_showpolys**: INT (default: 0)

Useful tool for mappers. 1 means show world polygon outlines for visible surfaces. 2 means show outlines for all surfaces in the PVS, even if they are hidden.

**gl_showcollisionmesh**: INT (default: 0)

Useful tool for mappers. Shows all the collision geometry for terrain in wireframe. 2 disables depth culling for this wireframe.

**gl_showdecals**: INT (default: 0)

Set this to 1 to show terrain decal bounding boxes. Set this to 2 to show terrain decals in wireframe.

**r_showpolycounts**: BOOL (default: 0)

Set this to 1 to show the polycount of each mesh, hovering over the mesh itself.

**r_showbox**: STRING (default: "")

Shows a box the size of the player's bounding box at the specified coordinates.

**r_tracetest**: INT (default: 0), **r_fasttracetest**: INT (default: 0), **r_tracebox**: STRING (default: ""), **r_tracetestlength**: FLOAT (default: 8192)

Performance and visual tests for the collision traces, starting at the player's origin along the view vector. `r_tracetestlength` is the trace length in game units.

**r_ragdoll_debug**: BOOL (default: 0)

Show ragdoll debug information.

---

## SERVER SIDE CVARs

### Server Connection CVARs

**hostname**: STRING (default: "noname")

Server name displayed to players and master server.

**maxclients**: INT (default: 1)

Maximum number of player slots on the server.

**timeout**: INT (default: 125)

Disconnect clients after this many seconds of inactivity.

**sv_iplimit**: INT (default: 3)

Maximum connections allowed per IP address (anti-spam).

**sv_reconnect_limit**: INT (default: 3)

Maximum number of reconnect attempts from same client.

**sv_public**: BOOL (default: 1)

List this server on master servers (0 = private/LAN only).

**sv_iplogfile**: STRING (default: "")

Path of a log file where clients' IP addresses will be appended; empty = disabled.

**sv_gamereport**: BOOL (default: 1)

When running a dedicated server, generate a post‑match game report file.

**maxspectators**: INT (default: 4)

Maximum number of spectators allowed on the server.

**needpass**: INT (default: 0)

Set automatically when the server requires a `password` or `spectator_password` (shown in the server list).

**maxentities**: INT (default: 1024)

Maximum number of entities in the game (latched, needs a map restart).

**g_teamgame**: INT (read-only)

Set automatically: 1 when the game is a team game (skin teams in `dmflags`, or CTF), otherwise 0.

**zombietime**: INT (default: 2)

Seconds a dropped client stays in the zombie state before its slot is freed.

**sv_ratelimit_status**: INT (default: 15)

Maximum number of server status requests answered per sampling period; further requests are dropped (flood protection).

**sv_enforcetime**: BOOL (default: 0)

Enable speed cheat detection: kick clients that claim significantly more simulation time than actually elapsed.

**sv_noreload**: BOOL (default: 0)

Legacy Quake 2 cvar (do not reload level state when re-entering a level); currently has no effect.

**filterban**: BOOL (default: 1)

If 1, IP addresses that match the ban list (`sv addip`) are refused. If 0, only addresses that match the list are allowed, which makes it easy to set up a private game.

**flood_msgs**: INT (default: 4), **flood_persecond**: INT (default: 4), **flood_waitdelay**: INT (default: 10)

Chat flood protection. A player who sends `flood_msgs` messages within `flood_persecond` seconds can not chat for `flood_waitdelay` seconds.

**motdfile**: STRING (default: "motd.txt")

File with the message of the day shown to joining players.

**motdforce**: INT (default: 0)

Number of frames the message of the day is forced on screen when a player joins. 0 = not forced.

**dedicated**: BOOL (default: 0, read-only)

Set by the `+set dedicated 1` command line option to run a dedicated server.

**protocol**: INT (default: 36, read-only)

Network protocol version of the server (shown in the server info).

**game**, **gamedir**: STRING

Game directory (mod) in use. Set `game` on the command line.

**gamedate**, **gamename**: STRING (read-only)

Build date and game name, shown in the server info.

**sv_botkickthreshold**: INT (default: 0)

Number of players the server tries to keep filled. Bots fill the empty slots up to this number, and are kicked one by one as human players join. When the number of human players reaches this value, all bots are removed. 0 disables this and allows `sv addbot` / `sv removebot`. Duel mode (`g_duel`) forces this behavior.

**sv_custombots**: INT (default: 0)

Selects a custom botinfo file; 0 uses the built‑in bots, other values load botinfo/custom<N>.tmp.

**bot_reactiontime**: FLOAT (default: 0.5)

Seconds a bot must have an enemy continuously in sight before it is allowed to open fire. 0 = instant/legacy behavior. Scaled by bot skill: higher-skill bots react faster (shorter effective delay), lower-skill bots react slower.

**bot_sightrange**: FLOAT (default: 1200)

Maximum distance (map units) at which bots will acquire and engage enemies. 0 = unlimited (legacy behavior). Lower this to stop bots from engaging across very open maps.

### Download CVARs

**allow_download**: BOOL (default: 1)

Enable file downloads from server for clients.

**allow_download_players**: BOOL (default: 0)

Allow downloading player model files.

**allow_download_models**: BOOL (default: 1)

Allow downloading map model files.

**allow_download_sounds**: BOOL (default: 1)

Allow downloading sound files.

**allow_download_maps**: BOOL (default: 1)

Allow downloading map files.

**sv_downloadurl**: STRING (default: "https://invader.alienarena.org/sv_downloadurl")

HTTP(S) URL for an alternative download location (avoids server bandwidth). Maps can be downloaded over https.

**auto_installmap**: BOOL (default: 1)

Automatically download and install map packs before loading maps. See also `allow_overwrite_maps`.

**allow_overwrite_maps**: BOOL (default: 0)

Used by the commands `installmap` and `installmodel`. If 1, existing downloaded map packs or model packs are overwritten and all files in them are extracted, replacing existing files.

### Game Mode CVARs

**deathmatch**: BOOL (default: 1)

Alien Arena is always deathmatch; the server sets this to 1.

**ctf**: BOOL (default: 0)

Enable Capture the Flag mode (team based).

**g_duel**: BOOL (default: 0)

Enable duel mode (one on one). Forces the bot auto-kick behavior of `sv_botkickthreshold`.

**g_tactical**: BOOL (default: 0)

Enable Tactical mode (team based, objective-based).

**skill**: INT (default: 1)

Difficulty level for bots (0=easy, 1=medium, 2=hard, 3=nightmare).

**g_autobalance**: BOOL (default: 0)

In team games, players are assigned to the team with fewer players automatically instead of choosing a team.

**g_reward**: INT (default: 20)

Reward points a player needs to earn the reward powerup.

**fraglimit**: INT (default: 0)

End game when player reaches this score (0 = unlimited).

**timelimit**: INT (default: 0)

End game after this many minutes (0 = unlimited).

**dmflags**: INT (default: 2097168)

Deathmatch flags bitmask controlling game rules. Add up the values of the options you want:

- 1: no health pickups
- 2: no item pickups
- 4: weapons stay
- 8: no falling damage
- 16: instant items
- 32: same level (do not advance to the next map)
- 64: skin teams
- 256: no friendly fire
- 512: spawn farthest
- 1024: force respawn
- 2048: no armor
- 4096: allow exit
- 8192: infinite ammo
- 16384: quad drop
- 65536: quad fire drop
- 131072: bots autosave nodes
- 262144: bot chat
- 524288: bot perfect aim (by default fuzzy)
- 1048576: disable bots
- 2097152: bot level advance (advance to the next map even if a bot won)

The default is 16 (instant items) + 2097152 (bot level advance).

**cheats**: BOOL (default: 0)

Enable cheat commands (powerful admin-only commands).

**g_spawnprotect**: INT (default: 2)

Number of seconds of invulnerability granted immediately after spawning.

**warmuptime**: INT (default: 0 for a single player game, 30 with `maxclients` above 1)

Warm‑up period in seconds before the match begins.

**anticamp**: BOOL (default: 0)

Enable anti‑camping punishment (players who stand still for too long are killed).

**camptime**: INT (default: 10)

Seconds of inactivity before anti‑camping punishment applies.

**ac_frames**: INT (default: 0)

Frame count threshold used by the anti‑camping algorithm.

**ac_threshold**: INT (default: 0)

Movement threshold for the anti‑camping check.

**g_randomquad**: BOOL (default: 0)

Spawn a random quad damage powerup in the level.

**g_mapvote**: BOOL (default: 0)

Enable in‑game map voting at the end of a level.

**g_voterand**: BOOL (default: 0)

Resolve a tie in the end-of-level map vote randomly.

**g_votemode**: INT (default: 0)

How the four maps of the end-of-level vote are chosen from `sv_maplist`: 0 = the next four maps in the list, any other value = four random maps from the list.

**g_votesame**: BOOL (default: 1)

Allow the currently playing map to be voted for again.

**sv_maplist**: STRING (default: "")

Space separated list of maps used for the map vote and for the rotation at the end of a level. Example: `set sv_maplist "dm-deathray dm-dynamo dm-turbo"`

**g_callvote**: BOOL (default: 1)

Allow players to initiate votes with `callvote`.

**g_select_empty**: BOOL (default: 0)

Allow selecting weapons that are out of ammo.

**g_spawnhealth**: INT (default: 125)

Health of a player at spawn.

**g_maxhealth**: INT (default: 100)

Maximum health a player can reach by normal pickups.

**g_maxbullets**: INT (default: 200), **g_maxshells**: INT (default: 100), **g_maxrockets**: INT (default: 50), **g_maxgrenades**: INT (default: 50), **g_maxcells**: INT (default: 200), **g_maxslugs**: INT (default: 50), **g_maxseekers**: INT (default: 1), **g_maxbombs**: INT (default: 1)

Maximum ammo a player can carry for each ammo type (shells are the alien smart grenades, grenades the napalm).

**wep_selfdmgmulti**: FLOAT (default: 1.0)

Multiplier for the damage players do to themselves with their own weapons.

**quickweap**: INT (default: 0)

Fast weapon switching.

**playerspeed**: BOOL (default: 0)

Use the faster maximum player speed (450 instead of 300), as in excessive mode.

### Antilag CVARs

**g_antilag_oneway_factor**: FLOAT (default: 0.5)

Factor to convert the round trip time (RTT) to a one-way ping estimate, which is the amount of rewind used for lag compensation. 0.5 = RTT/2.

**g_antilag_jitter_sigma**: FLOAT (default: 1.0)

Number of ping standard deviations added to a player's ping for lag compensation, so players with an unstable ping get more rewind. 0 disables it.

**g_antilag_jitter_max**: INT (default: 50)

Maximum extra ping in milliseconds that the ping jitter compensation may add.

**g_antilag_loss_bias_strength**: FLOAT (default: 0.8)

Per-player bias applied to the one-way factor based on the measured uplink/downlink packet loss asymmetry, to compensate players with packet loss. 0 disables it (the plain `g_antilag_oneway_factor` is used).

**g_antilag_loss_bias_confidence_threshold**: FLOAT (default: 0.3)

Combined uplink and downlink loss rate (0..1) needed to fully trust the loss-bias direction. Higher = more loss required before the bias saturates.

**g_antilag_low_ping_threshold**: INT (default: 50)

Ping threshold (ms) for the minimum lag compensation.

**g_antilag_high_ping_threshold**: INT (default: 250)

Ping threshold (ms) (the knee) where diminishing returns start for antilag compensation.

**g_antilag_max_ping**: INT (default: 300)

Maximum effective ping (ms) for lag compensation. Pings above this capped at this value.

**g_antilagdebug**: INT (default: 0)

Print extra console messages for antilag debugging. 0=off, 1=normal, 2=verbose.

**g_antilagprojectiles**: BOOL (default: 1)

Apply antilag compensation to projectile weapons.

### Special Game Modes

**sv_joustmode**: BOOL (default: 0)

Joust mode: players can jump in midair.

**excessive**: BOOL (default: 0)

Enable Excessive mode (high damage/fast-paced).

**sv_airaccelerate**: FLOAT (default: 0)

Air acceleration of players in deathmatch. 0 = default movement, higher values = faster air control.

**instagib**: BOOL (default: 0)

Enable Instagib mutator (single‑shot kills).

**rocket_arena**: BOOL (default: 0)

Enable Rocket Arena mutator (rocket jumps only).

**insta_rockets**: BOOL (default: 0)

Same as rocket_arena but instant‑respawn rockets.

**all_out_assault**: BOOL (default: 0)

Enable All‑Out Assault mutator (more weapons and faster pacing).

**low_grav**: BOOL (default: 0)

Lower gravity for all players.

**regeneration**: BOOL (default: 0)

Players slowly regenerate health over time.

**vampire**: BOOL (default: 0)

Health is drained from victims and given to attackers.

**grapple**: BOOL (default: 0)

Enable grapple hook tool.

**classbased**: BOOL (default: 0)

Enable class‑based team play.

**g_losehealth**: BOOL (default: 1)

Players lose one health point per second while their health is above `g_losehealth_num`. Not applied when `regeneration`, `excessive` or `vampire` is on.

**g_losehealth_num**: INT (default: 100)

Health level above which players lose health when `g_losehealth` is on.


### Physics/Game Balance CVARs

**sv_gravity**: INT (default: 800)

World gravity acceleration (standard units). Higher = heavier gravity.

**sv_maxvelocity**: INT (default: 2000)

Maximum velocity of any entity.

**sv_rollspeed**: FLOAT (default: 200), **sv_rollangle**: FLOAT (default: 2)

Speed at which the view rolls while strafing, and the maximum roll angle.

**run_pitch**: FLOAT (default: 0.002), **run_roll**: FLOAT (default: 0.005), **bob_up**: FLOAT (default: 0.005), **bob_pitch**: FLOAT (default: 0.002), **bob_roll**: FLOAT (default: 0.002)

View bobbing while running and walking.

**gun_x**, **gun_y**, **gun_z**: FLOAT (default: 0)

Offset of the weapon model.

### Server Tickrate and Netcode CVARs

**sv_tickrate**: INT (default: 100)

Server frame rate (ticks per second). Higher = more server updates but more CPU usage. A client's `cl_maxpackets` is limited by this.

**sv_netdebug**: BOOL (default: 0)

Print one diagnostic line per connected client per second: delta-window gap, forced full-resends, bytes sent, rate drops/overflows, and the current entity-cull angle. Intended to be left on only while diagnosing the stutter of a specific client.

**sv_resync_throttle_ms**: INT (default: 150)

Minimum milliseconds between forced full (non-delta) resyncs sent to a client that has fallen out of its delta-compression window. Without this, such a client would get the most expensive packet every tick for as long as the condition lasts. Ticks in between are skipped (as with a rate drop) instead of repeating the resync.

**sv_hitch_threshold_ms**: INT (default: 100)

If a server tick takes more than this many milliseconds (wall clock) beyond the per-tick budget of the current tickrate, print a breakdown: time in packet reading, game simulation, and total/worst-single-client send time. Independent of `sv_netdebug`. 0 disables this check.

### Entity Culling CVARs

The server culls entities that are too small (in angular size) to be seen by a client, which saves bandwidth. The cull angle adapts per client between `sv_entity_cull` (floor) and `sv_entity_cull_max` (ceiling), based on the number of visible entities and on the connection quality of the client. Clients can ask for a different value with the `entity_cull` userinfo cvar. Use `sv_entity_cull_status` to see the effective values.

**sv_entity_cull**: FLOAT (default: 1.0)

Cull entities smaller than this angular size (in degrees). Higher values = cull more aggressively. 0 disables culling. Acts as the minimum (floor) when adaptive scaling is active.

**sv_entity_cull_max**: FLOAT (default: 10.0)

Maximum angular size (in degrees) adaptive entity culling scales up to when the visible entity count of a client exceeds `sv_entity_budget_max`.

**sv_entity_budget**: INT (default: 80)

Number of visible entities per client (at sv_tickrate 100) below which culling stays at the `sv_entity_cull` minimum. Scaled automatically for other tickrates.

**sv_entity_budget_max**: INT (default: 150)

Number of visible entities per client (at sv_tickrate 100) at or above which culling reaches the `sv_entity_cull_max` ceiling. Scaled automatically for other tickrates.

**sv_entity_cull_vel_away_dot**: FLOAT (default: 0.0)

Dot-product threshold (-1 to 1) for the persistence rule: once an entity has been visible to a client, it stays exempt from angular-size culling unless the player moves away from it more directly than this. Lower = more lenient, higher = entities are re-culled sooner when the player leaves.

**sv_entity_cull_distress**: FLOAT (default: 0.1)

Fraction (0-1) of ticks a client needs a full resync (see `sv_resync_throttle_ms`) below which adaptive culling isn't increased for this reason. A client with a poor connection can need stronger culling than its entity count suggests. Whichever of this or the entity-count scaling is worse wins; a healthy connection is never affected.

**sv_entity_cull_distress_max**: FLOAT (default: 0.5)

Fraction (0-1) of ticks a client needs a full resync at or above which adaptive culling reaches `sv_entity_cull_max` due to connection distress alone, regardless of the visible entity count.

**sv_entity_cull_distress_cooldown_ms**: INT (default: 5000)

Milliseconds for the connection-distress part of adaptive culling to ease back down from `sv_entity_cull_max` to the floor once distress stops. It still rises immediately when distress increases. Prevents a fluctuating connection from making the angle flap up and down. 0 = relax immediately.

### Special Features

**g_dm_lights**: BOOL (default: 1)

Give players in non-team deathmatch a colored glow (not in tactical mode).

### Engine/Debug CVARs (server)

**timescale**: FLOAT (default: 1), **fixedtime**: INT (default: 0)

Scale the game time or force a fixed frame time (testing only).

**host_speeds**: BOOL (default: 0), **log_stats**: BOOL (default: 0), **showtrace**: BOOL (default: 0)

Print frame timing, log statistics or trace counts.

**logfile**: INT (default: 1), **logname**: STRING (default: "qconsole.log")

Write the console output to a log file (0 = off, 1 = buffered, 2 = flush after each print) and the name of that file.

**log_dest_udp**: STRING (default: "")

Also send the console log to this UDP address.

**developer**: INT (default: 0)

Print extra developer messages.

**map_noareas**: BOOL (default: 0)

Ignore area portals in the map (testing only).

**fasttrace_verify**: BOOL (default: 0)

Verify the fast trace results against the full trace (testing only).

**testcode**, **testmap**, **test**: (default: 0)

Internal test cvars.

---

## Network Rate Information

**Server-side rate clamping**: The server clamps all client `rate` values to the range **2500 to 75000 bytes/sec**. A client without a rate gets the default of 40000.

**Recommended rate settings**:
- Good internet connection: `set rate 75000` (use server maximum)
- Default: `set rate 40000`
- Limited bandwidth: `set rate 15000`
- Very poor connection: `set rate 5000` (may see more resyncs and lag compensation artifacts)

