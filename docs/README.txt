ALIEN ARENA 2007

1. Introduction
2. System Requirements
3. Getting Started
* Installation
* Basic Commands
* Deathmatch
* Connecting to a server
4 The CodeRED ACE Bot
5. Copyright Information


1.	INTRODUCTION

ALIEN ARENA 2007 is a standalone 3D first person online deathmatch shooter crafted from the original source code of  Quake II and Quake III,  released by id Software under the GPL license.  With features including 32 bit graphics, new particle engine and effects, light blooms, reflective water, hi resolution textures and skins, hi poly models, stain maps, ALIEN ARENA pushes the envelope of graphical beauty rivaling today's top games.

The game features 37 levels, which can be played online against other players, or against the built in CodeRED bots.  

Alien Arena offers CTF,  AOA(All Out Assault) mode, in which players can climb into vehicles to do battle, Deathball, and Team Core Assault.  Simply go into the multiplayer menu to start a server, change the game rules , and choose a map to play on.  You can also select from five different mutators(instagib, rocket arena, excessive, low grav, regen, and vampire) to further customize your game experience.

2.	SYSTEM REQUIREMENTS

1 ghz
256 mb RAM
500 mb hard disk space
3D Accelerator

3.	GETTING STARTED

1.	INSTALLATION

(Windows)

Installation has been made very simple.  After downloading, simply click on the alienarena2007.exe file and the installation will begin.  Follow the instructions of the Installshield setup program. If you are running Windows 2000 or XP, you should have administrative privileges.  If not, you may get some errors when the program tries to write the uninstall information to the registry.  If so, simply click "ignore", and the installation will continue without problem.

Once finished, you can click on the Alien Arena icon that has been placed on your desktop to start a game or the Alien Arena Server icon to start a dedicated server.

(Linux)

Simply unzip the archive in your usr/local/games folder or wherever you wish to place the game.  

Type ./crx to run the game, or ./crded to start a dedicated server or use the shortcuts installed in the menu.

Source files are included, so you may compile the binaries yourself if neccessary.  Please visit the forum if you have any questions regarding this.

2. BASIC COMMANDS

ALIEN ARENA works very much like Quake 2/3, with a few notable exceptions.

The player will have the following weapons:

1.	Blaster
2.	Alien Disruptor
3.	Chaingun
4.	Flame Thrower
5.	Rocket Launcher
6.	Alien Smart Gun
7.	Disruptor
8.	Alien Vaporizer

Weapons also have alternate firing modes.  In you controls menu, select your secondary fire key and this will allow you to use these modes.  

There are new powerups such as haste and sproing, in addition to the venerable quad damage and invulerabilty.  

The player will also have a flashlight, which is always present, and very useful.  The "F" key activates and de-activates the flashlight, or you can bind whatever key you wish to "flashlight" which will turn it on and off.

It would be advisable for you to change video settings for however you wish to view the game, and what gives you reasonable performance. The game's default settings are at a mid range setting, so try tweaking them or adding/removing effects depending on your framerate.  If you wish to see your framerate, you can type "set cl_drawfps 1" at the console(brought down using the ` key).  

ALIEN ARENA also allows for colored names just as Quake 3, using the ^ character followed by a number to set the color.

COMPLETING LEVELS

In Alien Arena you can play a single player tournament where  your goal will to be to reach the fraglimit before a bot does.  If you fail, you will be forced to repeat the level until you do. These rules can be changed in the menu system.  You can also play people online, where your goal is to reach the fraglimit before your opponents, or by scoring the most frags before the time limit is up.

In CTF mode, you'll be on a team, and trying to capture the enemy's flag and return it to your own flag's location.  The team reaching the capture limit first, wins.

In Deathball, the goal is to score as many points as possible, either by killing your opponents, or by finding the ball and shooting it into the goal(worth 10 frags).  When a player is inside of the deathball, he is defensless.

In Team Core Assault, your goal is to disable your enemy teams power nodes, then destroy the central spider node.  You can only do damage to the spider node when all other power nodes are disabled.

CONNECTING TO A SERVER

You can either select "join server" from the "multiplayer" menu and select a server in the list, or you can use the CodeRED Galaxy server browser(windows only).  In the in-game browser, servers are listed in order of ping, with lowest ping at the top of the list.  

4.	THE CODERED ACE BOT

Bots are a built in feature of ALIEN ARENA 2007.  Several bots are already configured for multiplayer games, and in Alien Arena, each level has a specific bot file for what bots are to be played in each level.

To add a bot, type "sv addbot name model/skin", and to remove a bot type "sv removebot namee". You can also add bots in the menu, in the deathmatch/bots flags area.

In your Deathmatch options, you can configure other options such as chatting, node saving, and aiming.

These bots are fully configurable using the Botconfigurator program.  You can change their skill levels, accuracy, weapon favoring, awareness, and chat strings.

Skill level 0 bots are quite easy to beat.  Skill level 1 bots are a little tougher, and do more dodging and are more accurate.  Skill level 2 bots do more advanced dodging, rocket jumping, and are even more accurate.  Level 3 bots are extremely skilled, and will strafe, and strafe jump around the level.  They will also taunt you after killing you.  If in Alien Arena, you play the single player tournament against the bots, selecting "easy" will make all bots skill level 0.  
Selecting "normal" will leave them at whatever level they are configured. Selecting "hard" will make them all move up 1 skill level.

5.	CREDITS

There is a long list of credits - ALIEN ARENA 2007 is greatly indebted to the following for it's creation and completion:

Design:  John Diamond with input from the community at large.
Programming:  John Diamond, Victor Luchits, Shane Bayer, Jan Rafaj, Tony Jackson, Kyle Hunter, Andres Mejia.
Models and skins:  John Diamond, Alex Perez, Shawn Keeth.
Maps:  John Diamond
Textures and Artwork:  John Diamond, Enki, Adam Saizlai
Sounds:  John Diamond, Sound Rangers
Music:  John Diamond, WhiteLipper, Wooden Productions, and SoundRangers
Linux Port: Shane Bayer
FreeBSD port: "Ale"
Gentoo portage: Paul Bredbury
Debian packaging: Andres Mejia
Alien Arena IRC Channel:  Astralsin

There are other major contributions from the community from QuakeSrc.org, including MrG, Psychospaz, Jalisko, Heffo, Chayfo, Dukey, Jitspoe, Knightmare, Barens, MH, and Carbon14.  Without this wonderful group of people, and the release of their accomplishments, many features would not have been possible.  A very special thanks goes out to the community members who contributed to the crosshair and hud contest.  A full list of those contributors can be found in the in-game credit list.

5.	COPYRIGHT INFORMATION

ALIEN ARENA 2007 and it's original content are a copyright of COR Entertainment, LLC.  

The source code of Alien Arena is Free Software. You can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.

It is only permissible to distrubute the game data(models, maps, textures, sound, etc) as a whole, and with the intention of being used with Alien Arena.  It is not permissible to distribute individual portions or items of the game data without express consent from COR Entertainment.

'rcon' and 'svstat' ruby scripts are Copyright (C) 2007 Tony Jackson and Licensed under the GNU Lesser General Public License

The Debian packaging is (C) 2007, Andres Mejia <mcitadel@gmail.com> and
is licensed under the GPL, see `/usr/share/common-licenses/GPL'.

Under no circumstances ALIEN ARENA 2007 as a whole be sold or used for profit, without express consent from COR Entertainment.  ALIEN ARENA 2007 may be included in free compilation CD's and similar packages without consent, provided it adheres to the above restrictions.

Contact:  http://red.planetarena.org




