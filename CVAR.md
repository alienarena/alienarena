# CVARs Documentation

## CLIENT SIDE CVARs

### Input/Control CVARs

**in_mouse**: BOOL (default: 1)

Enable/disable mouse input processing.

**in_poll_rate**: BOOL (default: 0)

Mouse poll frequency. 0=packet rate only (~62 Hz), 1=render rate (uncapped, highest responsiveness).

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

**cl_maxfps**: INT (default: 60)

Limit frames drawn per second using a busy-wait loop. Higher values = more responsive input.

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

Enable flickering light effects.

**cl_simpleitems**: BOOL (default: 0)

Use flat item icons instead of 3D models.

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

**rate**: INT (default: 25000, clamped to max 15000)

Bandwidth limit in bytes/second. Server enforces maximum of 15000.

**cl_timeout**: INT (default: 120)

Disconnect if no server response for this many seconds.

**cl_shownet**: INT (default: 0)

Show network traffic debug info. Higher values = more detail.

### User Info CVARs

**name**: STRING (default: "unnamed")

Player name (sent to server, case-sensitive when choosing servers).

**skin**: STRING (default: "male/grunt")

Character model and skin selection.

**hand**: INT (default: 0)

Weapon hand: 0=right, 1=center, 2=left.

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

Precache custom models on startup (uses more memory/time).

**cl_playtaunts**: BOOL (default: 1)

Play taunt sounds when players use them.

**cl_footsteps**: BOOL (default: 1)

Play footstep sounds when players move.

**cl_test**: BOOL (default: 0)

Enable test features/debug mode.

**cl_timedemo**: BOOL (default: 0)

Benchmark mode - runs demo at maximum speed and prints timing stats.

**cl_demoquit**: BOOL (default: 0)

Auto-quit after running a demo.

**cl_showplayernames**: INT (default: 0)

Display player names: 0=off, 1=teammates, 2=all players.

**cl_paindist**: INT (default: 0)

Sound distance for pain sounds.

**cl_explosiondist**: INT (default: 0)

Sound distance for explosion sounds.

**cl_raindist**: INT (default: 1)

Sound distance for rain/ambient sounds.

**cl_dm_lights**: BOOL (default: 0)

Enable Deathmatch-specific dynamic lighting optimizations.

**cl_disbeamclr**: INT (default: 0)

Disable specific beam color effects (0-7).

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

Filter server list to show only active servers.

**cl_stats_server**: STRING (default: "http://martianbackup.com")

Stats collection server URL for player rankings.

**cl_latest_game_version_url**: STRING (default: "http://invader.alienarena.org/version/crx_version")

URL for checking latest game version updates.

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

**sv_botkickthreshold**: INT (default: 0)

Ping threshold in milliseconds above which bots are automatically kicked. 0 disables.

**sv_custombots**: INT (default: 0)

Selects a custom botinfo file; 0 uses the built‑in bots, other values load botinfo/custom<N>.tmp.

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

**sv_downloadurl**: STRING (default: "http://invader.alienarena.org/sv_downloadurl")

HTTP URL for alternative download location (avoids server bandwidth).

**auto_installmap**: BOOL (default: 1)

Automatically install downloaded maps without player confirmation.

**allow_overwrite_maps**: BOOL (default: 0)

Allow download to overwrite existing map files.

### Game Mode CVARs

**deathmatch**: INT (default: 0)

Enable Deathmatch mode (free-for-all). 0=off, 1=on.

**ctf**: INT (default: 0)

Enable Capture the Flag mode.

**skill**: INT (default: 1)

Difficulty level for bots (0=easy, 1=medium, 2=hard, 3=nightmare).

**fraglimit**: INT (default: 0)

End game when player reaches this score (0 = unlimited).

**timelimit**: INT (default: 0)

End game after this many minutes (0 = unlimited).

**dmflags**: INT

Deathmatch flags bitmask controlling game rules (instant items, falling damage, etc).

**cheats**: BOOL (default: 0)

Enable cheat commands (powerful admin-only commands).

**g_spawnprotect**: INT (default: 2)

Number of seconds of invulnerability granted immediately after spawning.

**warmuptime**: INT (default: 0)

Warm‑up period in seconds before the match begins (30 by default in multiplayer).

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

Randomize the order of map choices when voting.

**g_votemode**: INT (default: 0)

Voting mode (rules for how maps are selected).

**g_votesame**: BOOL (default: 1)

Allow players to vote for the currently playing map.

**g_callvote**: BOOL (default: 1)

Allow players to initiate vote commands.

### Antilag CVARs

**g_antilag_oneway**: BOOL (default: 0)

Use one-way ping estimation for antilag (fairer to low-ping players).

**g_antilag_oneway_factor**: FLOAT (default: 0.5)

Antilag compensation multiplier (0.5 = 50% of Round Trip Time).

**g_antilag_low_ping_threshold**: INT (default: 50)

Minimum ping (ms) for lag compensation.

**g_antilag_high_ping_threshold**: INT (default: 250)

Ping threshold (ms) where diminishing returns start for antilag compensation.

**g_antilag_max_ping**: INT (default: 300)

Maximum effective ping (ms) for lag compensation. Pings above this capped at this value.

**g_antilagdebug**: INT (default: 0)

Debug antilag system. 0=off, 1=normal, 2=verbose.

**g_antilagprojectiles**: BOOL (default: 1)

Apply antilag compensation to projectile weapons.

### Special Game Modes

**sv_joustmode**: BOOL (default: 0)

Enable Joust mode (mid-air combat with unlimited jumping).

**g_tactical**: BOOL (default: 0)

Enable Tactical Ops mode (objective-based).

**excessive**: BOOL (default: 0)

Enable Excessive mode (high damage/fast-paced).

**sv_airaccelerate**: BOOL (default: 0)

Use alternative air movement physics (faster air strafing).

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

Enable the health‑loss mechanic (used by some mutators).

**g_losehealth_num**: INT (default: 100)

Amount of health lost per second when g_losehealth is active.


### Physics/Game Balance CVARs

**sv_gravity**: INT (default: 800)

World gravity acceleration (standard units). Higher = heavier gravity.

**sv_friction**: FLOAT (default: 6.0)

Ground friction multiplier. Higher = more friction, slower sliding.

### Server Tickrate CVARs

**sv_tickrate**: INT (default: 10)

Server frame rate (ticks per second). Higher = more server updates but more CPU usage.

### Bot CVARs

**sv_botphysics**: BOOL (default: 1)

Enable custom bot physics and movement.

### Special Features

**g_dm_lights**: BOOL (default: 1)

Enable Deathmatch-specific dynamic lighting optimizations.

### Statistics CVARs

**sv_ratelimit_status**: INT (default: 15)

Rate limit threshold for connection flood protection.

---

## Network Rate Information

**Server-side rate clamping**: The server clamps all client `rate` values to a maximum of **15000 bytes/sec**, even if clients report higher values. The minimum is 100 bytes/sec.

**Recommended rate settings**:
- Good internet connection: `set rate 15000` (use server maximum)
- Limited bandwidth: `set rate 8000` (conservative, reduces lag if connection unstable)
- Very poor connection: `set rate 4000` (minimal, may see lag compensation artifacts)

