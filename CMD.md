# COMMANDS Documentation

## CLIENT SIDE COMMANDS

### Connection Commands

**connect** `<address>[:port]`

Connect to a server. Example: `connect 192.168.1.100:27910` or `connect alienarena.com`

**disconnect**

Disconnect from current server and return to main menu.

**reconnect**

Reconnect to the last server you were connected to.

**rcon** `<password> <command>`

Execute a remote console command on the server (if you have rcon password). Example: `rcon mypass say Hello everyone`

### Demo Commands

**record** `<demoname>`

Start recording a demo with the given filename. Saved to demos/ folder. Example: `record ctf_battle`

**stop**

Stop recording current demo.

**timedemo** `<demoname>`

Play back a demo at maximum speed while timing performance. Useful for benchmarking.

### Input/Control Commands

**impulse** `<number>`

Trigger an impulse action (weapon selection, item use, etc). Common impulses:
- 10-20: Select weapons (10=blaster, 11=shotgun, etc)
- 100: Centerview (center look)
- 200: Zoom in
- 201: Zoom out

**force_centerview**

Force camera to center view (used in scripts).

### Client Status Commands

**status**

Show current connection and player status.

**pause**

Pause or unpause the game.

**ping**

Ping the current server to check latency.

**pingservers**

Ping all servers in the server list to get latency info.

### Server List Commands

**snd_restart**

Restart the sound system (fixes audio issues).

**skins**

Display list of available character skins.

**userinfo**

Display current user info (name, skin, hand, etc).

### Game State Commands

**quit**

Exit Alien Arena completely.

**map** `<mapname>`

Load a map directly (offline/demo mode). Example: `map dm-proxycon`

### Download/Install Commands

**download** `<filename>`

Download a file from the server. Example: `download dm-mymap.bsp`

**installmap** `<filename>`

Install a downloaded map file. Example: `installmap dm-mymap.bsp`

**installmodel** `<filename>`

Install a downloaded player model file.

### IRC Commands

**irc_connect**

Connect to IRC chat server (if configured).

**irc_disconnect**

Disconnect from IRC.

### Demo/Demo Playback

**demomap** `<mapname>`

Play a demo map (if available).

### Debug/Testing Commands

**cl_test** `<value>`

Enable testing features (internal development use).

**cl_shownet** `<value>`

Display network traffic: 0=off, 1=packets, 2=verbose

**showfps**

Toggle on-screen FPS display (alternative to cl_drawfps).

---

## SERVER SIDE COMMANDS

### Server Management

**map** `<mapname>`

Change the current map immediately. Example: `map dm-proxycon`

**startmap** `<mapname>`

Load initial map when server starts.

**demomap** `<mapname>`

Play a demo map as the current level.

**status**

Display server status and currently connected players with IDs.

**serverinfo**

Show all server cvars and their current values.

**heartbeat**

Send manual heartbeat to master servers (for server visibility).

**setmaster** `<servername>`

Set the master server for server registration.

**shutdown**

Shut down the server immediately (all players disconnected).

**killserver**

Gracefully shut down the server.

### Player Management

**kick** `<player_id_or_name>`

Kick a player from the server. Example: `kick 2` or `kick PlayerName`

**kickban** `<player_id>`

Kick and ban a player by IP address.

**unban** `<ip_address>`

Unban a previously banned IP address.

**ban** `<ip_address>`

Ban an IP address (prevents connections).

**status**

Shows player IDs which are needed for kick/ban commands.

**dumpuser** `<player_id>`

Display detailed information about a connected player.

### Announcements/Communication

**say** `<message>`

Broadcast a message to all players on the server. Example: `say Welcome to the CTF server!`

**sayteam** `<message>`

Send a message to a specific team only.

### Recording/Logging

**serverrecord** `<demoname>`

Start recording a server-side demo (records all players). Useful for tournament replays.

**serverstop**

Stop server-side demo recording.

### Server Command (sv subcommands)

**sv test**

Test command (internal use).

**sv addbot** `<name> [model/skin]`

Add a bot to the server with optional model/skin. Example: `sv addbot "BotName" "male/visor"`

**sv removebot** `<name>`

Remove a bot by name from the server. Example: `sv removebot BotName`

**sv addip** `<ip-mask>`

Add IP address to server ban list (supports masks). Example: `sv addip 192.168.1.100` or `sv addip 192.168.1.*`

**sv removeip** `<ip-mask>`

Remove IP address from ban list (must match exactly). Example: `sv removeip 192.168.1.100`

**sv listip**

Display all currently banned IP addresses.

**sv writeip**

Save current IP ban list to listip.cfg file for persistent storage.

**sv acedebug** `<on|off>`

Enable or disable ACE bot debug mode.

**sv savenodes**

Save bot navigation nodes to file.

### Configuration Commands (used in server.cfg)

**set** `<cvar> <value>`

Set a cvar value. Used in config files: `set maxclients 16`

**seta** `<cvar> <value>`

Set a cvar and archive it (save to config). `seta fraglimit 20`

**setf** `<cvar> <float_value>`

Set a float cvar value. `setf g_antilag_oneway_factor 0.75`

---

## ADMIN/RCON COMMANDS (Server-side, accessible via rcon or admin console)

### Voting System

**callvote** `<type> [arguments]`

Initiate a vote. Types: kickplayer, changemap, fraglimit, timelimit, etc.

**vote** `<yes|no>`

Vote yes or no on current vote.

### Banning System

**addip** `<ip_prefix>`

Add IP to server ban list. Example: `addip 192.168.*`

**removeip** `<ip_prefix>`

Remove IP from ban list.

**listip**

Show all currently banned IPs.

### Game Control

**fraglimit** `<value>`

Set frag limit (set fraglimit 50).

**timelimit** `<value>`

Set time limit in minutes (set timelimit 30).

**changemap** `<mapname>`

Change map with fade/transition effects.

**dmflags** `<value>`

Set game flags (fall damage, instant items, etc).

### Cheat Commands (when cheats enabled)

**god**

Godmode - invulnerability.

**noclip**

Noclip mode - walk through walls.

**notarget**

Monsters/enemies ignore you.

**give** `<item>`

Give yourself an item. Examples: health, armor, weapons, ammo.

**kill**

Suicide (remove yourself from game).

---

## Command Execution Order

### Config Files

Commands can be placed in config files (~/.alienarena/autoexec.cfg for personal config):

```
// Input settings
set in_poll_rate 1
set m_direct 1
set cl_maxfps 300
set sensitivity 3

// Network
set rate 15000

// Graphics
set gl_swapinterval 1

// User info
set name "PlayerName"
set skin "male/doom"
```

### Server Config (server.cfg)

```
// Server setup
set hostname "My Awesome Server"
set maxclients 16
set sv_public 1

// Game rules
set fraglimit 50
set timelimit 30
set dmflags 8

// Antilag
set g_antilag 1
set g_antilag_oneway 1
set g_antilag_oneway_factor 0.75
```

