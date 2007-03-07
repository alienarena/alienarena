<?php

/*
    ALIEN ARENA LIVE IMAGE GEN
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

include ('config.php');

// +++++++++ DB section start ++++++++++ //
$conn = mysql_connect($CONFIG['dbHost'], $CONFIG['dbUser'], $CONFIG['dbPass']) or die ('Error connecting to database');

mysql_select_db($CONFIG['dbName']);

/* Get time of last database update */
$query  = "SELECT lastupdated FROM stats WHERE id = '0'";
$result = mysql_query($query);
$row = mysql_fetch_array($result, MYSQL_ASSOC);
$lastupdated = $row['lastupdated'];
mysql_free_result($result);

/* Get all servers from last update which responded */
$query  = "SELECT serverid FROM serverlog WHERE time = '{$lastupdated}'";
$sv_result = mysql_query($query);
$numservers = mysql_num_rows($sv_result);
mysql_free_result($sv_result);

//echo "<p class=\"cdbody\">Currently {$numservers} servers are responding.</p>\n";

/* Get list of all players from last update */
$query  = "SELECT name, score, ping FROM playerlog WHERE time = '{$lastupdated}' AND ping != '0'";
$pl_result = mysql_query($query);
$numplayers = mysql_num_rows($pl_result);
mysql_free_result($pl_result);

//echo "<p class=\"cdbody\">{$numplayers} non-bot players:<p>\n";
mysql_close($conn);
// ++++++++++ DB section end ++++++++++ //

$text = "Live: {$numservers} servers, {$numplayers} players";

$image = imagecreatefromjpeg("sig.jpg");

$font = $CONFIG['font'];
$font_size = $CONFIG['fontsize'];

// RGB foreground color
$fgcolor = imagecolorallocate($image, 255,255,255);

// RGB drop shadow color
$bgcolor = imagecolorallocate($image, 0,150,0);

$xoffset = $CONFIG['xoffset'];
$yoffset = $CONFIG['yoffset'];
$shadowdepth = 1;

// text with drop shadow

ImageTTFText ($image, $font_size, 0, $xoffset + $shadowdepth, $yoffset + $shadowdepth, $bgcolor, $font, $text);
ImageTTFText ($image, $font_size, 0, $xoffset, $yoffset, $fgcolor, $font, $text);

header("Content-type: image/jpeg");
imagejpeg($image,"",$CONFIG['jpg_quality']);
imagedestroy($image);

?> 
