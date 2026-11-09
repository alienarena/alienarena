# COMMANDS Documentation

Use `cmdlist` in the console to list every command registered in your build, and `cvarlist` to list all cvars (see CVAR.md).

## CLIENT SIDE COMMANDS

### Connection Commands

**connect** `<address>[:port]`

Connect to a server. Example: `connect 192.168.1.100:27910` or `connect alienarena.com`

**disconnect**

Disconnect from current server and return to main menu.

**reconnect**

Reconnect to the last server you were connected to.

**rcon** `<command>`

Execute a remote console command on the server. Requires `rcon_password` (and `rcon_address` when not connected) to be set. Example: `set rcon_password mypass` then `rcon say Hello everyone`

**cmd** `<command>`

Forward a command string to the server as-is.

**changing**

Internal; shown while the server is changing level.

**precache**

Internal; sent by the server when loading a level, to precache the models, sounds and images.

**pingservers**

Ping all servers in the server list to get latency and player info.

**cl_jitters**

Show the inbound packet rate and jitter (current and session peak) of the connection to the server.

### Demo Commands

**record** `<demoname>`

Start recording a demo with the given filename. Saved to the demos/ folder. Example: `record ctf_battle`

**stop**

Stop recording the current demo.

**demomap** `<demoname>`

Play back a recorded demo. Use the `timedemo` cvar to play it in benchmark mode.

### Input/Control Commands

**+forward / -forward**, **+back / -back**, **+moveleft / -moveleft**, **+moveright / -moveright**, **+moveup / -moveup**, **+movedown / -movedown**

Movement. The `+` form starts the action when a key is pressed, the `-` form stops it when the key is released. Normally used in `bind`.

**+left / -left**, **+right / -right**, **+lookup / -lookup**, **+lookdown / -lookdown**

Turn or look using the keyboard (speed set by `cl_yawspeed` and `cl_pitchspeed`).

**+attack / -attack**, **+attack2 / -attack2**

Fire the current weapon (primary / secondary).

**+use / -use**

Use or activate (for example vehicles and some map objects).

**+speed / -speed**

Walk or run, depending on `cl_run`.

**+sneak / -sneak**

Sneak (move slowly and quietly).

**+strafe / -strafe**

Turn keys strafe instead of turn while held.

**+klook / -klook**, **+mlook / -mlook**

Keyboard look and mouse look while held.

**+zoom / -zoom**

Zoom in while held.

**centerview**

Center the view vertically.

**impulse** `<number>`

Send an impulse number to the game.

**bind** `<key> [command]`

Bind a command to a key. Without a command, shows the current binding. Example: `bind mouse1 +attack`

**unbind** `<key>`

Remove the binding of a key.

**unbindall**

Remove all key bindings.

**bindlist**

List all current key bindings.

**joy_advancedupdate**

(Windows) Apply the advanced joystick settings (`joy_adv*` cvars).

### Chat and Communication Commands

**say** `<message>`

Send a chat message to all players.

**say_team** `<message>`

Send a chat message to your team only.

**messagemode**

Open the chat input line for a message to all players.

**messagemode2**

Open the chat input line for a team message.

**messagemode3**

Open the chat input line for IRC chat.

**togglechat**

Show or hide the chat overlay (see `cl_chatlines`).

**vtaunt** `<1-5>`

Play a voice taunt of your player model. An invalid number plays a random one. Limited to one taunt every 2 seconds.

**wave** `<0-4>`

Play a gesture animation.

**chatbubble**

Show a chat bubble above your player's head.

### Client Status Commands

**players**, **playerlist**

List the players on the server with their score and ping.

**score**

Show the scoreboard.

**userinfo**

Display your current userinfo (name, skin, rate, hand, etc.).

**skins**

Display the skins of all connected players.

**pause**

Pause or unpause the game (single player or when allowed).

**viewpos**

Print your current position and view angle (used by mappers).

**info**

Show information about the current server.

### Item and Weapon Commands

**use** `<item>`

Use an item or switch to a weapon by name.

**drop** `<item>`

Drop an item (for example a flag or a weapon).

**weapnext**, **weapprev**, **weaplast**

Select the next, previous or previously used weapon.

**putaway**

Close the inventory or scoreboard.

**inven**, **invnext**, **invprev**, **invnextw**, **invprevw**, **invnextp**, **invprevp**, **invuse**, **invdrop**

Inventory handling: show the inventory, move the selection (any item, weapon or powerup) and use or drop the selected item.

**kill**

Suicide.

**teamselect**

Open the team selection.

### Game State Commands

**quit**

Exit Alien Arena completely.

**map** `<mapname>`

Start a local game on the given map. Example: `map dm-proxycon`

### Download/Install Commands

**download** `<filename>`

Download a file from the server.

**downloadhttp** `<filename>`

Same as `download`, but over HTTP(S) from the download URL of the server (`sv_downloadurl`).

**installmap** `<filename>`

Install a downloaded map pack. Existing files are only overwritten if `allow_overwrite_maps` is 1.

**installmodel** `<filename>`

Install a downloaded player model pack.

### IRC Commands

**irc_connect**

Connect to the IRC chat server (see the `cl_IRC_*` cvars).

**irc_quit**

Disconnect from IRC.

**irc_say** `<message>`

Say something in the IRC channel.

### Menu and Console Commands

**toggleconsole**

Open or close the console (Escape also closes it).

**menu_main**

Open the main menu.

**menu_quit**

Open the quit menu.

**clear**

Clear the console text.

**condump** `<filename>`

Write the console text to a file.

### Sound Commands

**snd_restart**

Restart the sound system (fixes audio issues).

**playsound** `<sound>`

Play a sound file.

**stopsound**

Stop all sounds.

**soundinfo**

Print information about the sound device.

**soundlist**

List the loaded sounds.

### Video Commands

**vid_restart**

Restart the video system, to apply changed video cvars.

**vid_modelist**

List the available video modes.

**vid_front**

(Windows) Bring the game window to the front.

**screenshot**

Take a screenshot (format set by `gl_screenshot_type`).

**gl_strings**

Print the OpenGL vendor, renderer and extensions.

**imagelist**

List the loaded images.

**modellist**

List the loaded models.

**sky** `<basename> [rotate] [x y z]`

Change the sky of the current map.

**timerefresh** `<1|2>`

Benchmark the renderer: 1 = rotate through 360 degrees, 2 = fixed view.

**loading**

Show the loading screen.

### Console and Scripting Commands

**exec** `<filename>`

Execute the commands in a config file. Example: `exec autoexec.cfg`

**echo** `<text>`

Print text to the console.

**alias** `<name> [commands]`

Create a command alias. Without commands, shows the alias. Example: `alias fastswitch "weapnext; wait; weapnext"`

**unalias** `<name>`

Remove an alias.

**wait**

Wait one frame before executing the rest of the commands.

**set** `<cvar> <value> [u|s|g]`

Set a cvar and create it if needed. The optional flag makes it a userinfo (`u`), serverinfo (`s`) or gameinfo (`g`) cvar. Example: `set sensitivity 4`

**setenv** `<name> [value]`

Set or show an operating system environment variable.

**help** `<cvar>`

Show the type, value and description of a cvar.

**cvarlist**

List all cvars.

**cmdlist**

List all commands.

**path**

Show the search paths of the file system.

**dir** `<pattern>`

List the files matching a pattern. Example: `dir maps/*.bsp`

### Debug Commands

**z_stats**

Print the memory zone statistics.

**error** `<message>`

Force an engine error (testing only).

**hang** `<msec>`

Simulate a hang of the game for the given number of milliseconds (testing only).

---

## SERVER SIDE COMMANDS

These commands can be typed in the console of a dedicated server, in the console of a listen server, in a config file, or sent from a client with `rcon`.

### Server Management

**map** `<mapname>`

Change to the given map immediately. Example: `map dm-proxycon`

**startmap** `<mapname>`

Load the initial map when the server starts (used in server.cfg).

**demomap** `<demoname>`

Play a demo as the current level.

**status**

Display the map and the connected players with their IDs, score, ping and address.

**serverinfo**

Show all serverinfo cvars and their current values.

**heartbeat**

Send a manual heartbeat to the master servers (for server visibility).

**setmaster** `<master1> [master2 ...]`

Set the master servers the server registers with (on a dedicated server) and make the server public.

**killserver**

Shut down the server and return to the main menu or exit (dedicated).

**say** `<message>`

(Dedicated server only) Broadcast a message to all players as the server console.

### Player Management

**kick** `<userid>`

Kick a player from the server. The userid is the number shown by `status`. Example: `kick 2`

**dumpuser** `<userid>`

Display the userinfo of a connected player.

### Recording

**serverrecord** `<demoname>`

Start recording a server-side demo (records all players). Useful for tournament replays.

**serverstop**

Stop server-side demo recording.

### Diagnostics

**sv_jitters**

Show per-player network statistics: ping, ping deviation, packets per second, jitter, uplink and downlink packet loss, and the antilag bias and factor in use.

**sv_entity_cull_status**

Show the adaptive entity culling settings and the effective cull angle per client (see the `sv_entity_cull*` cvars).

### Server Commands (sv subcommands)

These are handled by the game module: `sv <subcommand> [arguments]`.

**sv test**

Test command (internal use).

**sv addbot** `[name] [skin]`

Add a bot to the server with optional name and model/skin. Example: `sv addbot "BotName" "enforcer/blue"`. Disabled when bots are disabled in `dmflags` or when `sv_botkickthreshold` is set or `g_duel` is on.

**sv removebot** `<name>`

Remove a bot by name from the server. Example: `sv removebot BotName`. Disabled under the same conditions as `sv addbot`.

**sv addip** `<ip-mask>`

Add an IP address to the server ban list (addresses with fewer than four parts act as a prefix mask). Example: `sv addip 192.168.1.100`

**sv removeip** `<ip-mask>`

Remove an IP address from the ban list (must match exactly). Example: `sv removeip 192.168.1.100`

**sv listip**

Display all currently banned IP addresses.

**sv writeip**

Save the current IP ban list to listip.cfg for persistent storage.

**sv acedebug** `<on|off>`

Enable or disable ACE bot debug mode (enables the node editing commands below).

**sv savenodes**

Save the bot navigation nodes to file.

### Bot Node Editing Commands

Available in the game console of a player, only after `sv acedebug on`.

**addnode** `<type>`, **removelink** `<from> <to>`, **addlink** `<from> <to>`, **showpath** `<node>`, **shownode** `<node>`, **hidenode** `<node>`, **findnode** `[node]`, **movenode** `<node> <x> <y> <z>`

Create, link, inspect and move bot navigation nodes.

### Configuration Commands (used in server.cfg)

**set** `<cvar> <value> [u|s|g]`

Set a cvar value. Used in config files: `set maxclients 16`

---

## PLAYER COMMANDS HANDLED BY THE SERVER

These are sent by clients to the game module (and can be typed in the console of a connected client).

### Voting System

**callvote** `<command> [argument]`

Start a vote. Not possible during warmup or while another vote is running, and only if `g_callvote` is 1. Supported commands are `kick <player>`, `kickban <player>`, `fraglimit <value>`, `timelimit <value>` and `map <mapname>`. Example: `callvote map dm-proxycon`

**vote** `<1|2>`

Vote yes (1) or no (2) on the current vote. During the end-of-level map vote (`g_mapvote`), the number selects the map.

### Cheat Commands (when `cheats` is 1)

**god**

Godmode - invulnerability.

**noclip**

Noclip mode - walk through walls.

**notarget**

Monsters/enemies ignore you.

**give** `<item>`

Give yourself an item. Examples: `give all`, `give health`, `give weapons`, `give ammo`, `give armor`, or an item name.

---

## Command Execution Order

### Config Files

Commands can be placed in config files (autoexec.cfg in your user game folder for personal config; config.cfg is written by the game):

```
// Input settings
set m_direct 1
set cl_maxfps 300
set sensitivity 3

// Network
set rate 40000
set cl_maxpackets 100

// Graphics
set gl_swapinterval 1

// User info
set name "PlayerName"
set skin "enforcer/default"
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

// Network and antilag
set sv_tickrate 100
set g_antilagprojectiles 1
set g_antilag_oneway_factor 0.5
```
