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

/* Basic remote portal for Alien Arena database - outputs number of servers and players */

include ('config.php');

/* This variable selects, at output time, if the data is presented as a flattened array, or put out as simple text */
$settings['format'] = 0;  /* 0 = default, flattened array, 1 = text */

/* Case-sensitive parameters to parse result for */
$parameters = array('format');
/* Parse url for valid parameters */
foreach ($_GET as $key => $value)
{
	if(!(array_search($key, $parameters) === FALSE))
		$settings[$key] = $value;
}

$data = array();

// +++++++++ DB section start ++++++++++ //
$conn = mysql_connect($CONFIG['dbHost'], $CONFIG['dbUser'], $CONFIG['dbPass']) or die ('Error connecting to database');

mysql_select_db($CONFIG['dbName']);

/* Get time of last database update */
$query  = "SELECT lastupdated FROM stats WHERE id = '0'";
$result = mysql_query($query);
$row = mysql_fetch_array($result, MYSQL_ASSOC);
$data['lastupdated'] = $lastupdated = $row['lastupdated'];
mysql_free_result($result);

/* Get all servers from last update which responded */
$query  = "SELECT serverid FROM serverlog WHERE time = '{$lastupdated}'";
$sv_result = mysql_query($query);
$data['servers'] = mysql_num_rows($sv_result);
mysql_free_result($sv_result);

/* Get list of all players from last update */
$query  = "SELECT name, score, ping FROM playerlog WHERE time = '{$lastupdated}' AND ping != '0'";
$pl_result = mysql_query($query);
$data['players'] = mysql_num_rows($pl_result);
mysql_free_result($pl_result);

mysql_close($conn);
// ++++++++++ DB section end ++++++++++ //

if($settings['format'] == 0)
	echo serialize($data);
else
{
	foreach($data as $key => $value)
		echo $key."=".$value."\n";
}

?> 
