#include "lua_script.h"

#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <SDL3/SDL.h>

#include <lauxlib.h>
#include <lualib.h>

/* Return the first path where `filename` exists, searching:
   1. <basepath>/scripts/<filename>   (debug: next to binary)
   2. <basepath>/../Resources/scripts/<filename>  (release: macOS bundle)
   3. fallback: <filename> as-is (cwd-relative)
   The result is stored in a static buffer; caller should not free it. */
static const char *find_script(const char *filename)
{
    static char resolved[1024];
    const char *base = SDL_GetBasePath();
    if (!base) {
        return filename;  /* can't resolve, try cwd-relative */
    }

    const char *search_dirs[] = {
        "scripts",
        "../Resources/scripts",
        NULL
    };

    for (int i = 0; search_dirs[i]; i++) {
        SDL_snprintf(resolved, sizeof(resolved), "%s%s/%s",
                     base, search_dirs[i], filename);
        if (SDL_GetPathInfo(resolved, NULL)) {
            SDL_free((void *)base);
            return resolved;
        }
    }

    /* Not found in any known location */
    SDL_free((void *)base);
    SDL_Log("Lua script not found alongside binary, trying cwd: %s", filename);
    return filename;
}

static void load_file(lua_State *L, const char *path)
{
    if (luaL_dofile(L, path) != LUA_OK) {
        SDL_Log("Error loading %s: %s", path, lua_tostring(L, -1));
        lua_pop(L, 1);
    }
}

/* Scan <basepath>/mods/ and load every .lua file found. */
static void load_mods(lua_State *L)
{
    const char *base = SDL_GetBasePath();
    if (!base) return;

    char mods_dir[1024];
    SDL_snprintf(mods_dir, sizeof(mods_dir), "%smods", base);
    SDL_free((void *)base);

    DIR *dir = opendir(mods_dir);
    if (!dir) return;  /* no mods folder — that's fine */

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        const char *name = entry->d_name;
        size_t len = strlen(name);
        if (len <= 4 || SDL_strcasecmp(name + len - 4, ".lua") != 0) {
            continue;
        }

        char fullpath[2048];
        SDL_snprintf(fullpath, sizeof(fullpath), "%s/%s", mods_dir, name);
        SDL_Log("Loading mod: %s", fullpath);
        load_file(L, fullpath);
    }

    closedir(dir);
}

lua_State* lua_script_init(void)
{
    lua_State *L = luaL_newstate();
    if (!L) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
            "Failed to create Lua state");
        return NULL;
    }

    luaL_openlibs(L);

    /* Load main script */
    load_file(L, find_script("app.lua"));

    /* Load any mods found next to the binary */
    load_mods(L);

    return L;
}

void lua_script_reload(lua_State *L)
{
    SDL_Log("Reloading Lua script...");
    load_file(L, find_script("app.lua"));
    load_mods(L);
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
