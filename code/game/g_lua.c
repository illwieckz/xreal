/*
===========================================================================
Copyright (C) 2006 Robert Beckebans <trebor_7@users.sourceforge.net>

This file is part of XreaL source code.

XreaL source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

XreaL source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with XreaL source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//
// g_lua.c

#include "g_local.h"

#ifdef LUA
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
static lua_State *g_luaState = NULL;
#endif

#define MAX_LUAFILE 32768

/*
=================
G_LoadLuaScript
=================
*/
void G_LoadLuaScript(gentity_t * ent, const char *filename)
{
    int             len;
    fileHandle_t    f;
    char            buf[MAX_LUAFILE];
    
    G_Printf("...loading '%s'\n", filename);
    
    len = trap_FS_FOpenFile(filename, &f, FS_READ);
    if(!f)
    {
        trap_Printf(va(S_COLOR_RED "file not found: %s\n", filename));
        return;
    }
    
    if(len >= MAX_LUAFILE)
    {
        trap_Printf(va(S_COLOR_RED "file too large: %s is %i, max allowed is %i\n", filename, len, MAX_LUAFILE));
        trap_FS_FCloseFile(f);
        return;
    }
    
    trap_FS_Read(buf, len, f);
    buf[len] = 0;
    trap_FS_FCloseFile(f);
    
#ifdef LUA
    if(luaL_loadbuffer(g_luaState, buf, strlen(buf), filename))
        G_Printf("G_RunLuaScript: cannot load lua file: %s\n", lua_tostring(g_luaState, -1));
    
    if(lua_pcall(g_luaState, 0, 0, 0))
        G_Printf("G_RunLuaScript: cannot pcall: %s\n", lua_tostring(g_luaState, -1));
#endif
}

/*
============
G_InitLua_Global

load multiple global precreated lua scripts
============
*/
void G_InitLua_Global()
{
	int             numdirs;
	int             numFiles;
	char            filename[128];
	char            dirlist[1024];
	char           *dirptr;
	int             i;
	int             dirlen;

	numFiles = 0;

	numdirs = trap_FS_GetFileList("scripts/lua/global/", ".lua", dirlist, 1024);
	dirptr = dirlist;
	for(i = 0; i < numdirs; i++, dirptr += dirlen + 1)
	{
		dirlen = strlen(dirptr);
		strcpy(filename, "scripts/lua/global/");
		strcat(filename, dirptr);
		numFiles++;

		// load the file
		G_LoadLuaScript(NULL, filename);
	}

	Com_Printf("%i global files parsed\n", numFiles);
}

/*
============
G_InitLua_Local

load multiple lua scripts from the maps directory
============
*/
void G_InitLua_Local(char mapname[MAX_STRING_CHARS])
{
	int             numdirs;
	int             numFiles;
	char            filename[128];
	char            dirlist[1024];
	char           *dirptr;
	int             i;
	int             dirlen;

	numFiles = 0;

	numdirs = trap_FS_GetFileList(va("scripts/lua/%s", mapname), ".lua", dirlist, 1024);
	dirptr = dirlist;
	for(i = 0; i < numdirs; i++, dirptr += dirlen + 1)
	{
		dirlen = strlen(dirptr);
		strcpy(filename, va("scripts/lua/%s/", mapname));
		strcat(filename, dirptr);
		numFiles++;

		// load the file
		G_LoadLuaScript(NULL, filename);

	}

	Com_Printf("%i local files parsed\n", numFiles);
}

/*
============
G_InitLua
============
*/
void G_InitLua()
{
	char            buf[MAX_STRING_CHARS];

	G_Printf("------- Game Lua Initialization -------\n");

#ifdef LUA
	g_luaState = lua_open();

	// Lua standard lib
	luaopen_base(g_luaState);
	luaopen_string(g_luaState);

	// Quake lib
	luaopen_entity(g_luaState);
	luaopen_game(g_luaState);
	luaopen_qmath(g_luaState);
	luaopen_vector(g_luaState);
#endif

	// load global scripts
	G_Printf("global lua scripts:\n");
	G_InitLua_Global();

	// load map-specific lua scripts
	G_Printf("map specific lua scripts:\n");
	trap_Cvar_VariableStringBuffer("mapname", buf, sizeof(buf));
	G_InitLua_Local(buf);

	G_Printf("-----------------------------------\n");


}



/*
=================
G_ShutdownLua
=================
*/
void G_ShutdownLua()
{
	G_Printf("------- Game Lua Finalization -------\n");

#ifdef LUA
	if(g_luaState)
	{
		lua_close(g_luaState);
		g_luaState = NULL;
	}
#endif

	G_Printf("-----------------------------------\n");
}


/*
=================
G_RunLuaFunction
=================
*/
void G_RunLuaFunction(const char *func, const char *sig, ...)
{
#ifdef LUA
	va_list         vl;
	int             narg, nres;	// number of arguments and results
	lua_State      *L = g_luaState;

	if(!func || !func[0])
		return;

	va_start(vl, sig);
	lua_getglobal(L, func);		// get function

	// push arguments
	narg = 0;
	while(*sig)
	{
		switch (*sig++)
		{
			case 'f':
				// float argument
				lua_pushnumber(L, va_arg(vl, float));

				break;

			case 'i':
				// int argument
				lua_pushnumber(L, va_arg(vl, int));

				break;

			case 's':
				// string argument
				lua_pushstring(L, va_arg(vl, char *));

				break;

			case 'e':
				// entity argument
				lua_pushentity(L, va_arg(vl, gentity_t *));
				break;

			case 'v':
				// vector argument
				lua_pushvector(L, va_arg(vl, vec_t *));
				break;

			case '>':
				goto endwhile;

			default:
				G_Printf("G_RunLuaFunction: invalid option (%c)\n", *(sig - 1));
		}
		narg++;
		luaL_checkstack(L, 1, "too many arguments");
	}
  endwhile:

	// do the call
	nres = strlen(sig);			// number of expected results
	if(lua_pcall(L, narg, nres, 0) != 0)	// do the call
		G_Printf("G_RunLuaFunction: error running function `%s': %s\n", func, lua_tostring(L, -1));

	// retrieve results
	nres = -nres;				// stack index of first result
	while(*sig)
	{							// get results
		switch (*sig++)
		{

			case 'f':
				// float result
				if(!lua_isnumber(L, nres))
					G_Printf("G_RunLuaFunction: wrong result type\n");
				*va_arg(vl, float *) = lua_tonumber(L, nres);

				break;

			case 'i':
				// int result
				if(!lua_isnumber(L, nres))
					G_Printf("G_RunLuaFunction: wrong result type\n");
				*va_arg(vl, int *) = (int)lua_tonumber(L, nres);

				break;

			case 's':
				// string result
				if(!lua_isstring(L, nres))
					G_Printf("G_RunLuaFunction: wrong result type\n");
				*va_arg(vl, const char **) = lua_tostring(L, nres);

				break;

			default:
				G_Printf("G_RunLuaFunction: invalid option (%c)\n", *(sig - 1));
		}
		nres++;
	}
	va_end(vl);
#endif
}


/*
=================
G_DumpLuaStack
=================
*/
void G_DumpLuaStack()
{
#ifdef LUA
	int             i;
	lua_State      *L = g_luaState;
	int             top = lua_gettop(L);

	for(i = 1; i <= top; i++)
	{
		// repeat for each level
		int             t = lua_type(L, i);

		switch (t)
		{
			case LUA_TSTRING:
				// strings
				G_Printf("`%s'", lua_tostring(L, i));
				break;

			case LUA_TBOOLEAN:
				// booleans
				G_Printf(lua_toboolean(L, i) ? "true" : "false");
				break;

			case LUA_TNUMBER:
				// numbers
				G_Printf("%g", lua_tonumber(L, i));
				break;

			default:
				// other values
				G_Printf("%s", lua_typename(L, t));
				break;

		}
		G_Printf("  ");			// put a separator
	}
	G_Printf("\n");				// end the listing
#endif
}
