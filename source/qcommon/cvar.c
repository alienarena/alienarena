/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2010 COR Entertainment, LLC.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
// cvar.c -- dynamic variable tracking

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "qcommon.h"

cvar_t	*cvar_vars;

/*
============
Cvar_InfoValidate
============
*/
static qboolean Cvar_InfoValidate (char *s)
{
	if (strstr (s, "\\"))
		return false;
	if (strstr (s, "\""))
		return false;
	if (strstr (s, ";"))
		return false;
	return true;
}

/*
============
Cvar_FindVar
============
*/
static cvar_t *Cvar_FindVar (char *var_name)
{
	cvar_t		*var;
	unsigned int	i, hash_key;

	COMPUTE_HASH_KEY( hash_key, var_name , i );
	for (var=cvar_vars ; var && var->hash_key <= hash_key ; var=var->next)
		if (var->hash_key == hash_key && !Q_strcasecmp (var_name, var->name))
			return var;

	return NULL;
}

/*
============
Cvar_VariableValue
============
*/
float Cvar_VariableValue (char *var_name)
{
	cvar_t	*var;

	var = Cvar_FindVar (var_name);
	if (!var)
		return 0;
	return var->value;
}


/*
============
Cvar_VariableString
============
*/
char *Cvar_VariableString (char *var_name)
{
	cvar_t *var;

	var = Cvar_FindVar (var_name);
	if (!var)
		return "";
	return var->string;
}


/*
============
Cvar_CompleteVariable
============
*/
char *Cvar_CompleteVariable (char *partial)
{
	cvar_t		*cvar;
	int		len, i;
	unsigned int	hash_key;

	len = strlen(partial);
	if (!len)
		return NULL;

	// check exact match
	COMPUTE_HASH_KEY( hash_key, partial , i );
	for (cvar=cvar_vars ; cvar && cvar->hash_key <= hash_key; cvar=cvar->next)
		if (cvar->hash_key == hash_key && !Q_strcasecmp (partial,cvar->name))
			return cvar->name;

	// check partial match
	for (cvar=cvar_vars ; cvar ; cvar=cvar->next)
		if (!Q_strncasecmp (partial,cvar->name, len))
			return cvar->name;

	return NULL;
}


/*
============
Cvar_Allocate

Creates a new variable's record
============
*/
inline static cvar_t *Cvar_Allocate(char *var_name, char *var_value, int flags, unsigned int hash_key)
{
	cvar_t *nvar;

	nvar = Z_Malloc (sizeof(cvar_t));
	nvar->name = CopyString (var_name);
	nvar->string = CopyString (var_value);
	nvar->modified = true;
	nvar->value = atof (nvar->string);
	nvar->integer = atoi (nvar->string);
	nvar->flags = flags;
	nvar->hash_key = hash_key;

	return nvar;
}


/*
============
Cvar_AddBetween

Adds a variable between two others.
============
*/
inline static cvar_t *Cvar_AddBetween(char *var_name, char *var_value, int flags, unsigned int hash_key,cvar_t **prev, cvar_t *next)
{
	cvar_t *nvar;

	// variable needs to be created, check parameters
	if (!var_value)
		return NULL;

	if (flags & (CVAR_USERINFO | CVAR_SERVERINFO))
	{
		if (!Cvar_InfoValidate (var_value))
		{
			Com_Printf("invalid info cvar value\n");
			return NULL;
		}
	}

	// create the variable
	nvar = Cvar_Allocate( var_name , var_value , flags , hash_key );

	// link the variable in
	nvar->next = next;
	*prev = nvar;

	return nvar;
}


/*
============
Cvar_Get

If the variable already exists, the value will not be set
The flags will be or'ed in if the variable exists.
============
*/
cvar_t *Cvar_Get (char *var_name, char *var_value, int flags)
{
	cvar_t		*var, **prev;
	unsigned int	i, hash_key;

	// validate variable name
	if (flags & (CVAR_USERINFO | CVAR_SERVERINFO))
	{
		if (!Cvar_InfoValidate (var_value))
		{
			Com_Printf("invalid info cvar value\n");
			return NULL;
		}
	}

	// try finding the variable
	prev = &cvar_vars;
	COMPUTE_HASH_KEY( hash_key , var_name , i );
	for (var = cvar_vars ; var && var->hash_key <= hash_key ; var=var->next)
	{
		if (var->hash_key == hash_key && !Q_strcasecmp (var_name, var->name))
		{
			var->flags |= flags;
			return var;
		}
		prev = &( var->next );
	}

	return Cvar_AddBetween(var_name , var_value , flags , hash_key , prev , var);
}


/*
============
Cvar_FindOrCreate

This function is mostly similar to Cvar_Get(name,value,0). However, it starts
by attempting to find the variable, as there are no flags. In addition, it
returns a boolean depending on whether the variable was found or created, and
sets the pointer to the variable through a parameter.
============
*/
inline static qboolean Cvar_FindOrCreate (
		char *var_name, char *var_value, int flags, cvar_t **found)
{
	cvar_t		*var, **prev;
	unsigned int	i, hash_key;

	// try finding the variable
	prev = &cvar_vars;
	COMPUTE_HASH_KEY( hash_key , var_name , i );
	for (var = cvar_vars ; var && var->hash_key <= hash_key ; var=var->next)
	{
		if (var->hash_key == hash_key && !Q_strcasecmp (var_name, var->name))
		{
			var->flags |= flags;
			*found = var;
			return true;
		}
		prev = &( var->next );
	}

	*found = Cvar_AddBetween(var_name , var_value , flags , hash_key , prev , var);
	return false;
}


/*
============
Cvar_Set2
============
*/
cvar_t *Cvar_Set2 (char *var_name, char *value, qboolean force)
{
	cvar_t	*var;

	// Find the variable; if it does not exist, create it and return at once
	if (!Cvar_FindOrCreate (var_name, value, 0, &var))
		return var;

	if (var->flags & (CVAR_USERINFO | CVAR_SERVERINFO))
	{
		if (!Cvar_InfoValidate (value))
		{
			Com_Printf("invalid info cvar value\n");
			return var;
		}
	}

	if (!force)
	{
		if (var->flags & CVAR_NOSET)
		{
			Com_Printf ("%s is write protected.\n", var_name);
			return var;
		}

		if (var->flags & CVAR_LATCH)
		{
			if (var->latched_string)
			{
				if (strcmp(value, var->latched_string) == 0)
					return var;
				Z_Free (var->latched_string);
			}
			else
			{
				if (strcmp(value, var->string) == 0)
					return var;
			}

			if (Com_ServerState())
			{
				Com_Printf ("%s cvar will be changed for next game.\n", var_name);
				Com_Printf( " startmap <mapname> will start next game.\n");
				var->latched_string = CopyString(value);
			}
			else
			{
				var->string = CopyString(value);
				var->value = atof (var->string);
				var->integer = atoi (var->string);
				if (!Q_strcasecmp(var->name, "game"))
				{
					FS_SetGamedir (var->string);
					FS_ExecAutoexec ();
				}
			}
			return var;
		}
	}
	else
	{
		if (var->latched_string)
		{
			Z_Free (var->latched_string);
			var->latched_string = NULL;
		}
	}

	if (!strcmp(value, var->string))
		return var;		// not changed

	var->modified = true;

	if (var->flags & CVAR_USERINFO)
		userinfo_modified = true;	// transmit at next oportunity

	Z_Free (var->string);	// free the old value string

	var->string = CopyString(value);
	var->value = atof (var->string);
	var->integer = atoi(var->string);

	return var;
}

/*
============
Cvar_ForceSet
============
*/
cvar_t *Cvar_ForceSet (char *var_name, char *value)
{
	return Cvar_Set2 (var_name, value, true);
}

/*
============
Cvar_Set
============
*/
cvar_t *Cvar_Set (char *var_name, char *value)
{
	return Cvar_Set2 (var_name, value, false);
}

/*
============
Cvar_FullSet
============
*/
cvar_t *Cvar_FullSet (char *var_name, char *value, int flags)
{
	cvar_t	*var;

	if (!Cvar_FindOrCreate (var_name, value, flags, &var))
		return var;

	var->modified = true;

	if (var->flags & CVAR_USERINFO)
		userinfo_modified = true;	// transmit at next oportunity

	Z_Free (var->string);	// free the old value string

	var->string = CopyString(value);
	var->value = atof (var->string);
	var->integer = atoi(var->string);
	var->flags = flags;

	return var;
}

/*
============
Cvar_SetValue
============
*/
void Cvar_SetValue (char *var_name, float value)
{
	char	val[32];

	if (value == (int)value)
		Com_sprintf (val, sizeof(val), "%i",(int)value);
	else
		Com_sprintf (val, sizeof(val), "%f",value);
	Cvar_Set (var_name, val);
}


/*
============
Cvar_GetLatchedVars

Any variables with latched values will now be updated
============
*/
void Cvar_GetLatchedVars (void)
{
	cvar_t	*var;

	for (var = cvar_vars ; var ; var = var->next)
	{
		if (!var->latched_string)
			continue;

		// observe when latched cvars are updated
		Com_Printf("Updating latched cvar %s from %s to %s\n",
				var->name, var->string, var->latched_string );

		Z_Free (var->string);
		var->string = var->latched_string;
		var->latched_string = NULL;
		var->value = atof(var->string);
		var->integer = atoi(var->string);
		if (!Q_strcasecmp(var->name, "game"))
		{
			FS_SetGamedir (var->string);
			FS_ExecAutoexec ();
		}
	}
}

/*
============
Cvar_Command

Handles variable inspection and changing from the console
============
*/
qboolean Cvar_Command (void)
{
	cvar_t			*v;

// check variables
	v = Cvar_FindVar (Cmd_Argv(0));
	if (!v)
		return false;

// perform a variable print or set
	if (Cmd_Argc() == 1)
	{
		Com_Printf ("\"%s\" is \"%s\"\n", v->name, v->string);
		return true;
	}

	Cvar_Set (v->name, Cmd_Argv(1));
	return true;
}


/*
============
Cvar_Set_f

Allows setting and defining of arbitrary cvars from console
============
*/
void Cvar_Set_f (void)
{
	int		c;
	int		flags;

	c = Cmd_Argc();
	if (c != 3 && c != 4)
	{
		Com_Printf ("usage: set <variable> <value> [u / s]\n");
		return;
	}

	if (c == 4)
	{
		if (!strcmp(Cmd_Argv(3), "u"))
			flags = CVAR_USERINFO;
		else if (!strcmp(Cmd_Argv(3), "s"))
			flags = CVAR_SERVERINFO;
		else
		{
			Com_Printf ("flags can only be 'u' or 's'\n");
			return;
		}
		Cvar_FullSet (Cmd_Argv(1), Cmd_Argv(2), flags);
	}
	else
		Cvar_Set (Cmd_Argv(1), Cmd_Argv(2));
}


/*
============
Cvar_WriteVariables

Appends lines containing "set variable value" for all variables
with the archive flag set to true.
============
*/
void Cvar_WriteVariables (char *path)
{
	cvar_t	*var;
	char	buffer[1024];
	FILE	*f;

	f = fopen (path, "a");
	for (var = cvar_vars ; var ; var = var->next)
	{
		if (var->flags & CVAR_ARCHIVE)
		{
			Com_sprintf (buffer, sizeof(buffer), "set %s \"%s\"\n", var->name, var->string);
			fprintf (f, "%s", buffer);
		}
	}
	fclose (f);
}

/*
============
Cvar_List_f

============
*/
void Cvar_List_f (void)
{
	cvar_t	*var;
	int		i;

	i = 0;
	for (var = cvar_vars ; var ; var = var->next, i++)
	{

		if (var->flags & CVAR_ARCHIVE)
			Com_Printf ("*");
		else
			Com_Printf (" ");
		if (var->flags & CVAR_USERINFO)
			Com_Printf ("U");
		else
			Com_Printf (" ");
		if (var->flags & CVAR_SERVERINFO)
			Com_Printf ("S");
		else
			Com_Printf (" ");
		if (var->flags & CVAR_NOSET)
			Com_Printf ("-");
		else if (var->flags & CVAR_LATCH)
			Com_Printf ("L");
		else
			Com_Printf (" ");
		Com_Printf (" %s \"%s\"\n", var->name, var->string);
	}
	Com_Printf ("%i cvars\n", i);
}


qboolean userinfo_modified;


char	*Cvar_BitInfo (int bit)
{
	static char	info[MAX_INFO_STRING];
	cvar_t	*var;

	info[0] = 0;

	for (var = cvar_vars ; var ; var = var->next)
	{
		if (var->flags & bit)
		{
			Info_SetValueForKey (info, var->name, var->string);
		}
	}
	return info;
}

// returns an info string containing all the CVAR_USERINFO cvars
char	*Cvar_Userinfo (void)
{
	return Cvar_BitInfo (CVAR_USERINFO);
}

// returns an info string containing all the CVAR_SERVERINFO cvars
char	*Cvar_Serverinfo (void)
{
	return Cvar_BitInfo (CVAR_SERVERINFO);
}

/*
============
Cvar_Init

Reads in all archived cvars
============
*/
void Cvar_Init (void)
{
	Cmd_AddCommand ("set", Cvar_Set_f);
	Cmd_AddCommand ("cvarlist", Cvar_List_f);

}
