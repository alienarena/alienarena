<?php

/*
    ALIEN ARENA WEB SERVER BROWSER
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
*/

include 'config.php';

include 'support.php';

include 'servers.php';
include 'maps.php';
include 'players.php';

$control = BuildControl();  /* Get config from URL line */

Generate_HTML_Headers($CONFIG['baseurl'], $CONFIG['title']);

$filename = GetFilename();

$conn = mysql_connect($CONFIG['dbHost'], $CONFIG['dbUser'], $CONFIG['dbPass']) or die ('Cannot connect to the database because: ' . mysql_error());
mysql_select_db($CONFIG['dbName']) or die ('Database not found on host');

echo "<a href=\"http://www.cooldark.com/\"><img border=0 width=400 height=90 alt=\"Server Browser\" src=\"img/server_browser_cooldark.jpg\"></a><br>\n";
//echo "<font class=\"cdtitle\">{$CONFIG['title']} ({$CONFIG['version']})</font>\n";
//echo "<p class=\"cdsubtitle\">Recovering from mysql server crash - stats valid in 24h</p>\n";

//echo "<p class=\"cdbody\">Please help boost the number of players on Alien Arena 2007 by <a target=_blank href=\"http://digg.com/gaming_news/Alien_Arena_2007_Awesome_Quake_3_Style_FPS_for_FREE\">digging it</a> here.</p>";
//echo "<p class=\"cdbody\">Please <a target=_blank href=\"http://www.download.com/Alien-Arena-2007/3640-7453_4-10630629.html?tag=tab_ur\">submit a review</a> now that version 6.03 of Alien Arena has been released.</p>";
echo "<p class=\"cdsubtitle\"> - <a href=\"{$filename}?action=showlive\">Live information</a> - <a href=\"{$filename}?action=serverstats\">Server stats</a> - <a href=\"{$filename}?action=playerstats\">Player stats</a> - <a href=\"{$filename}?action=mapstats\">Map stats</a> - </p>\n";
CheckDBLive();

switch ($control['action'])
{
	case 'showlive':
		echo "<img alt=\"Player graph\" width={$CONFIG['graphwidth']} height={$CONFIG['graphheight']} src=\"graph.php?show=players\"><br><br>\n";
		echo "<img alt=\"Server graph\" width={$CONFIG['graphwidth']} height={$CONFIG['graphheight']} src=\"graph.php?show=servers\"><br>\n";
		//GenerateLivePlayerTable(&$control);
		GenerateLiveServerTable(&$control);
	break;
	case 'serverstats':
		/* Section to build table of servers with most playertime*/
		echo "<p class=\"cdsubtitle\">Server usage in the last {$control['history']} hours</p>\n";
		echo "<img alt=\"Server graph\" width={$CONFIG['graphwidth']} height={$CONFIG['graphheight']} src=\"graph.php?show=servers&history={$control['history']}\"><br><br>\n";
		GenerateServerTable(&$control);
		GenerateNumResultsSelector(&$control);
		GenerateSearchInput("serversearch", "Server search");
	break;
	case 'playerstats':
		echo "<p class=\"cdsubtitle\">Player activity in the last {$control['history']} hours</p>\n";
		echo "<img alt=\"Player graph\" width={$CONFIG['graphwidth']} height={$CONFIG['graphheight']} src=\"graph.php?show=players&history={$control['history']}\"><br><br>\n";
		GeneratePlayerTable(&$control);
		GenerateNumResultsSelector(&$control);
		GenerateSearchInput("playersearch", "Player search");
	break;
	case 'mapstats':
		/* Get list of most played maps */
		echo "<p class=\"cdsubtitle\">Map usage in the last {$control['history']} hours</p>\n";
		GenerateMapTable(&$control);
		GenerateNumResultsSelector(&$control);
		GenerateSearchInput("mapsearch", "Map search");
	break;
	
	case 'serverinfo':
		GenerateServerInfo(&$control);
		GenerateSearchInput("serversearch", "Find another server");
	break;
	case 'playerinfo':
		GeneratePlayerInfo(&$control);
		GenerateSearchInput("playersearch", "Find another player");
	break;
	case 'mapinfo':
		GenerateMapInfo(&$control);
		GenerateSearchInput("mapsearch", "Find another map");
	break;

	case 'serversearch':
		DoServerSearch(&$control);
		GenerateSearchInput("serversearch", "Search for another server");
	break;
	case 'playersearch':
		DoPlayerSearch(&$control);
		GenerateSearchInput("playersearch", "Search for another player");
	break;
	case 'mapsearch':
		DoMapSearch(&$control);
		GenerateSearchInput("mapsearch", "Search for another map");
	break;

	break;
	default:
	break;
}

Generate_HTML_Footers();
mysql_close($conn);
?>

