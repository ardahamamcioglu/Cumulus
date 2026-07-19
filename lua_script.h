#ifndef CUMULUS_LUA_SCRIPT_H
#define CUMULUS_LUA_SCRIPT_H

#include <lua.h>

/* Init new Lua state, open libs, load app.lua */
lua_State* lua_script_init(void);

/* Reload app.lua (calls luaL_dofile again) */
void lua_script_reload(lua_State *L);

/* Call global Lua function "update()" if it exists */
void lua_script_update(lua_State *L);

/* Close Lua state */
void lua_script_shutdown(lua_State *L);

#endif /* CUMULUS_LUA_SCRIPT_H */
