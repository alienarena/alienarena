<?php

/*
    ALIEN ARENA SERVER SCANNER FOR WEB SERVER BROWSER
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

$CONFIG['dbHost'] = "localhost";
$CONFIG['dbName'] = "dbname_here";
$CONFIG['dbUser'] = "dbuser_here";
$CONFIG['dbPass'] = "dbpass_here";
$CONFIG['dbExpire'] = 60 * 60 * 24;  /* Duration (in seconds) to keep server and player data in the database */

error_reporting (E_NONE);

?>
