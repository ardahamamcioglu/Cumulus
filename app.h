#ifndef CUMULUS_APP_H
#define CUMULUS_APP_H

#include <SDL3/SDL.h>
#include <lua.h>
#include "microui.h"

typedef struct AppContext {
    SDL_Window* window;
    SDL_GPUDevice* device;
    lua_State *L;
    mu_Context mu_ctx;
} AppContext;

/* Init SDL, window, GPU device, microui, Lua. Returns NULL on failure. */
AppContext* app_init(void);

/* Per-frame: Lua update, UI, render */
SDL_AppResult app_iterate(AppContext *ctx);

/* Handle SDL event. Returns SDL_APP_SUCCESS to quit, SDL_APP_CONTINUE otherwise. */
SDL_AppResult app_event(AppContext *ctx, SDL_Event *event);

/* Shutdown everything, free context */
void app_quit(AppContext *ctx);

#endif /* CUMULUS_APP_H */
