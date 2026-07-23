#ifndef CUMULUS_LUA_SCRIPT_H
#define CUMULUS_LUA_SCRIPT_H

#include <lua.h>

/* Init new Lua state, open libs, load scripts/app.lua.
   Searches alongside the binary first (scripts/ for debug builds,
   ../Resources/scripts/ for release macOS bundles), falling back
   to the current working directory.
   Also loads any .lua files found in a mods/ folder next to the binary. */
lua_State* lua_script_init(void);

/* Reload scripts/app.lua and mods (calls luaL_dofile again) */
void lua_script_reload(lua_State *L);

/* Call global Lua function "update()" if it exists */
void lua_script_update(lua_State *L);

/* Close Lua state */
void lua_script_shutdown(lua_State *L);

#endif /* CUMULUS_LUA_SCRIPT_H */
