#define SDL_MAIN_USE_CALLBACKS 1

#include <stdlib.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

typedef struct AppContext {
    SDL_Window* window;
    SDL_GPUDevice* device;
    lua_State *L;
} AppContext;

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appState, int argc, char *argv[]) {

    SDL_SetAppMetadata("Cumulus", "0.0.1",
      "com.arda.cumulus");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
            "Could not initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_WindowFlags windowFlags =
      //SDL_WINDOW_HIGH_PIXEL_DENSITY |
		SDL_WINDOW_RESIZABLE;

    SDL_Window* window = SDL_CreateWindow(
      "Cumulus", 800, 600, windowFlags);

    if (window == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
            "Could not create window: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_GPUShaderFormat shaderFormats =
      SDL_GPU_SHADERFORMAT_SPIRV |
      SDL_GPU_SHADERFORMAT_DXIL |
      SDL_GPU_SHADERFORMAT_MSL;

    SDL_GPUDevice* device = SDL_CreateGPUDevice(shaderFormats,
      false, NULL);
    if (device == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
            "Could not create GPU device: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_Log("Using %s GPU implementation.",
      SDL_GetGPUDeviceDriver(device));

    if (!SDL_ClaimWindowForGPUDevice(device, window)) {
        SDL_Log("SDL_ClaimWindowForGPUDevice failed: %s",
            SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_SetGPUSwapchainParameters(device, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC);

    AppContext* context = SDL_malloc(sizeof(AppContext));
    context->window = window;
    context->device = device;
    context->L = luaL_newstate();

    // 1. Open standard libraries (print, math, string, etc.)
    luaL_openlibs(context->L);

    // Load Lua Scripts Here
    const char* scriptPath = "app.lua";
    if (luaL_dofile(context->L, scriptPath) != LUA_OK) {
        const char* errorMessage = lua_tostring(context->L, -1);
        SDL_Log("Error loading %s: %s", scriptPath, errorMessage);
        lua_pop(context->L, 1); // Pop error message from stack
    }

    *appState = context;

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appState)
{
  AppContext* context = appState;

  lua_getglobal(context->L, "update");
    if (lua_isfunction(context->L, -1)) {
        if (lua_pcall(context->L, 0, 0, 0) != LUA_OK) {
            SDL_Log("Lua Runtime Error: %s", lua_tostring(context->L, -1));
            lua_pop(context->L, 1); // Pop the error message
        }
    } else {
        lua_pop(context->L, 1); // Pop the non-function value (e.g., nil)
    }

  // Generally speaking, this is where you'd track frame times,
  // update your game state, etc. I'll be doing that in later
  // posts.

  // Once you're ready to start drawing, begin by grabbing a
  // command buffer and a reference to the swapchain texture.
    SDL_GPUCommandBuffer* cmdBuf;
    cmdBuf = SDL_AcquireGPUCommandBuffer(context->device);
    if (cmdBuf == NULL) {
    SDL_Log("SDL_AcquireGPUCommandBuffer failed: %s",
        SDL_GetError());
    return SDL_APP_FAILURE;
    }

  // As I understand it, _this_ is where it's going to wait for
  // Vsync, not in the loop that calls SDL_AppIterate.
  SDL_GPUTexture* swapchainTexture;
  if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmdBuf,
          context->window, &swapchainTexture, NULL, NULL)) {
    SDL_Log("SDL_WaitAndAcquireGPUSwapchainTexture: %s",
        SDL_GetError());
    return SDL_APP_FAILURE;
  }

  // With the command buffer and swapchain texture in hand, we
  // can begin and end our render pass
  if (swapchainTexture != NULL) {
    // There are a lot more options you can set for a render
    // pass, see SDL_GPUColorTargetInfo in the SDL documentation
    // for more.
    // https://wiki.libsdl.org/SDL3/SDL_GPUColorTargetInfo
    SDL_GPUColorTargetInfo targetInfo = {
        // The texture that we're drawing in to
        .texture = swapchainTexture,

        // Whether to cycle that texture. See
        // https://moonside.games/posts/sdl-gpu-concepts-cycling/
        // for more info
        .cycle = true,

        // Clear the texture to a known color before drawing
        .load_op = SDL_GPU_LOADOP_CLEAR,

        // Keep the rendered output
        .store_op = SDL_GPU_STOREOP_STORE,

        // And here's the clear color, a nice green.
        .clear_color = {0.16f, 0.47f, 0.34f, 1.0f}};

    // Begin and end the render pass. With no drawing commands,
    // this will clear the swapchain texture to the color
    // provided above and nothing else.
    SDL_GPURenderPass* renderPass;
    renderPass = SDL_BeginGPURenderPass(cmdBuf, &targetInfo,
        1, NULL);

    /* Draw UI */

    SDL_EndGPURenderPass(renderPass);
  }

  // And finally, submit the command buffer for drawing. The
  // driver will take over at this point and do all the rendering
  // we've asked it to.
  SDL_SubmitGPUCommandBuffer(cmdBuf);

  // That's it for this frame.
  //
  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appState, SDL_Event* event)
{
    // SDL_EVENT_QUIT is sent when the main (last?) application
    // window closes.
    if (event->type == SDL_EVENT_QUIT) {
        // SDL_APP_SUCCESS means we're making a clean exit.
        // SDL_APP_FAILURE would mean something went wrong.
        return SDL_APP_SUCCESS;
    }

    // For convenience, I'm also quitting when the user presses the
    // escape key. It makes life easier when I'm testing on a Steam
    // Deck.
    if (event->type == SDL_EVENT_KEY_DOWN) {
        if (event->key.key == SDLK_ESCAPE) {
            return SDL_APP_SUCCESS;
        }

        // Press R to reload Lua
        if (event->key.key == SDLK_R) {
            AppContext* context = (AppContext*)appState;
            SDL_Log("Reloading Lua script...");

            // Re-run the file to update functions
            if (luaL_dofile(context->L, "app.lua") != LUA_OK) {
                SDL_Log("Error reloading app.lua: %s", lua_tostring(context->L, -1));
                lua_pop(context->L, 1); // Pop error message
            }
        }
    }

    // Nothing else to do, so just continue on with the next frame
    // or event.
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appState, SDL_AppResult result)
{
    AppContext* context = (AppContext*)appState;

    if (context != NULL) {

        // Close Lua state
        if (context->L) {
            lua_close(context->L);
        }

        if (context->device != NULL) {
            if (context->window != NULL) {
                SDL_ReleaseWindowFromGPUDevice(context->device,
                    context->window);
                SDL_DestroyWindow(context->window);
            }

            SDL_DestroyGPUDevice(context->device);
        }

        SDL_free(context);
    }

    SDL_Quit();
}
