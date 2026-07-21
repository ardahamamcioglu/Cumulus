#define SDL_MAIN_USE_CALLBACKS 1

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "app.h"

SDL_AppResult SDL_AppInit(void **appState, int argc, char *argv[])
{
    AppContext *ctx = app_init();
    if (!ctx)
    {
        return SDL_APP_FAILURE;
    }
    *appState = ctx;
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appState)
{
    return app_iterate((AppContext *)appState);
}

SDL_AppResult SDL_AppEvent(void *appState, SDL_Event *event)
{
    return app_event((AppContext *)appState, event);
}

void SDL_AppQuit(void *appState, SDL_AppResult result)
{
    (void)result;
    app_quit((AppContext *)appState);
}
