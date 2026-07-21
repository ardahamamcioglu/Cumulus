#include "lua_script.h"

#include <stdlib.h>
#include <SDL3/SDL.h>

#include <lauxlib.h>
#include <lualib.h>

lua_State* lua_script_init(void)
{
    lua_State *L = luaL_newstate();
    if (!L) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
            "Failed to create Lua state");
        return NULL;
    }

    luaL_openlibs(L);

    if (luaL_dofile(L, "app.lua") != LUA_OK) {
        SDL_Log("Error loading app.lua: %s", lua_tostring(L, -1));
        lua_pop(L, 1);
    }

    return L;
}

void lua_script_reload(lua_State *L)
{
    SDL_Log("Reloading Lua script...");
    if (luaL_dofile(L, "app.lua") != LUA_OK) {
        SDL_Log("Error reloading app.lua: %s", lua_tostring(L, -1));
        lua_pop(L, 1);
    }
}

void lua_script_update(lua_State *L)
{
    lua_getglobal(L, "Update");
    if (lua_isfunction(L, -1)) {
        if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
            SDL_Log("Lua Runtime Error: %s", lua_tostring(L, -1));
            lua_pop(L, 1);
        }
    } else {
        lua_pop(L, 1);
    }
}

void lua_script_shutdown(lua_State *L)
{
    if (L) {
        lua_close(L);
    }
}
