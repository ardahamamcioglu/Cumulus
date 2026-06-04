#define SDL_MAIN_USE_CALLBACKS 1
#define CLAY_IMPLEMENTATION

#include <stdlib.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <clay.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

typedef struct AppContext {
    SDL_Window* window;
    SDL_GPUDevice* device;
    lua_State *L;
    Clay_Arena clayArena;
} AppContext;

static void ClayErrorHandler(Clay_ErrorData errorData) {
    SDL_Log("Clay error: %.*s",
        errorData.errorText.length,
        errorData.errorText.chars);
}

static Clay_Dimensions ClayMeasureTextStub(
    Clay_StringSlice text,
    Clay_TextElementConfig *config,
    void *userData)
{
    (void)text;
    (void)config;
    (void)userData;

    // Return zero size — text won't render, but layout won't crash.
    // Replace with real font measurement later.
    return (Clay_Dimensions){ 0, 0 };
}

// Called once at startup to initialize the application.
SDL_AppResult SDL_AppInit(void **appState, int argc, char *argv[]) {

    SDL_SetAppMetadata("Cumulus", "0.0.1",
      "com.arda.cumulus");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
            "Could not initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // Create a resizable window (no high-DPI flag for now).
    SDL_WindowFlags windowFlags = SDL_WINDOW_RESIZABLE;

    SDL_Window* window = SDL_CreateWindow(
      "Cumulus", 800, 600, windowFlags);

    if (window == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
            "Could not create window: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // Request SPIR-V, DXIL, and MSL support for cross-platform shaders.
    SDL_GPUShaderFormat shaderFormats =
      SDL_GPU_SHADERFORMAT_SPIRV |
      SDL_GPU_SHADERFORMAT_DXIL |
      SDL_GPU_SHADERFORMAT_MSL;

    // Create GPU device (debug mode disabled for now).
    SDL_GPUDevice* device = SDL_CreateGPUDevice(shaderFormats,
      false, NULL);
    if (device == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
            "Could not create GPU device: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_Log("Using %s GPU implementation.",
      SDL_GetGPUDeviceDriver(device));

    // Attach the window to the GPU device for rendering.
    if (!SDL_ClaimWindowForGPUDevice(device, window)) {
        SDL_Log("SDL_ClaimWindowForGPUDevice failed: %s",
            SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // Configure swapchain — SDR with VSync enabled.
    SDL_SetGPUSwapchainParameters(device, window,
      SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
      SDL_GPU_PRESENTMODE_VSYNC);

    // Allocate Clay's internal memory arena.
    uint32_t minMem = Clay_MinMemorySize();
    Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(
      minMem, SDL_malloc(minMem));

    // Initialize Clay layout system with current window dimensions.
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    Clay_Initialize(arena,
      (Clay_Dimensions){ (float)w, (float)h },
      (Clay_ErrorHandler){ .errorHandlerFunction = ClayErrorHandler });

    // Stub text measurement — no fonts yet, just return zeros.
    Clay_SetMeasureTextFunction(ClayMeasureTextStub, NULL);

    // Allocate and populate the application context.
    AppContext* context = SDL_malloc(sizeof(AppContext));
    context->window = window;
    context->device = device;
    context->L = luaL_newstate();
    context->clayArena = arena;

    // Open standard Lua libraries (print, math, string, etc.).
    luaL_openlibs(context->L);

    // Load the Lua app script.
    const char* scriptPath = "app.lua";
    if (luaL_dofile(context->L, scriptPath) != LUA_OK) {
        const char* errorMessage = lua_tostring(context->L, -1);
        SDL_Log("Error loading %s: %s", scriptPath, errorMessage);
        lua_pop(context->L, 1);
    }

    *appState = context;

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appState)
{
  AppContext* context = appState;

  // --- 1. Feed mouse input into Clay ---
  float mx, my;
  Uint32 buttons = SDL_GetMouseState(&mx, &my);
  Clay_SetPointerState((Clay_Vector2){ mx, my },
    buttons & SDL_BUTTON_LMASK);

  // --- 2. Begin layout ---
  Clay_BeginLayout();

  // --- 3. Declare UI elements ---
  CLAY(CLAY_ID("OuterContainer"),
      CLAY_LAYOUT_DEFAULT)
  {
      CLAY_TEXT(CLAY_STRING("Hello from Clay!"),
          CLAY_TEXT_CONFIG({
              .fontSize = 128,
              .textColor = {255, 255, 255, 255}
          }));
  }

  // --- 4. End layout and collect render commands ---
  Clay_RenderCommandArray renderCommands = Clay_EndLayout();

  // Call Lua update function if one exists.
  lua_getglobal(context->L, "update");
  if (lua_isfunction(context->L, -1)) {
      if (lua_pcall(context->L, 0, 0, 0) != LUA_OK) {
          SDL_Log("Lua Runtime Error: %s",
            lua_tostring(context->L, -1));
          lua_pop(context->L, 1);
      }
  } else {
      lua_pop(context->L, 1);
  }

  // Acquire a command buffer from the GPU device.
  SDL_GPUCommandBuffer* cmdBuf;
  cmdBuf = SDL_AcquireGPUCommandBuffer(context->device);
  if (cmdBuf == NULL) {
    SDL_Log("SDL_AcquireGPUCommandBuffer failed: %s",
        SDL_GetError());
    return SDL_APP_FAILURE;
  }

  // Wait for the next swapchain texture (blocks on VSync).
  SDL_GPUTexture* swapchainTexture;
  if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmdBuf,
          context->window, &swapchainTexture, NULL, NULL)) {
    SDL_Log("SDL_WaitAndAcquireGPUSwapchainTexture: %s",
        SDL_GetError());
    return SDL_APP_FAILURE;
  }

  if (swapchainTexture != NULL) {
    // Configure the color target: clear to green, then present.
    // See https://wiki.libsdl.org/SDL3/SDL_GPUColorTargetInfo
    // and https://moonside.games/posts/sdl-gpu-concepts-cycling/
    SDL_GPUColorTargetInfo targetInfo = {
        .texture = swapchainTexture,
        .cycle = true,
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_STORE,
        .clear_color = {0.16f, 0.47f, 0.34f, 1.0f}};

    SDL_GPURenderPass* renderPass;
    renderPass = SDL_BeginGPURenderPass(cmdBuf, &targetInfo,
      1, NULL);

    // Draw UI: iterate render commands and issue GPU draws.
    for (int32_t i = 0; i < renderCommands.length; i++) {
      Clay_RenderCommand* cmd = Clay_RenderCommandArray_Get(
        &renderCommands, i);
      // Switch on cmd->commandType, issue GPU draws.
    }

    SDL_EndGPURenderPass(renderPass);
  }

  // Submit everything to the GPU driver for rendering.
  SDL_SubmitGPUCommandBuffer(cmdBuf);

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appState, SDL_Event* event)
{
    // Quit when the main window closes.
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }

    // Quit on Escape (useful for testing on Steam Deck, etc.).
    if (event->type == SDL_EVENT_KEY_DOWN) {
        if (event->key.key == SDLK_ESCAPE) {
            return SDL_APP_SUCCESS;
        }

        // Press R to reload Lua scripts at runtime.
        if (event->key.key == SDLK_R) {
            AppContext* context = (AppContext*)appState;
            SDL_Log("Reloading Lua script...");

            if (luaL_dofile(context->L, "app.lua") != LUA_OK) {
                SDL_Log("Error reloading app.lua: %s",
                  lua_tostring(context->L, -1));
                lua_pop(context->L, 1);
            }
        }
    }

    // Update Clay layout dimensions on window resize.
    if (event->type == SDL_EVENT_WINDOW_RESIZED) {
        Clay_SetLayoutDimensions((Clay_Dimensions){
            (float)event->window.data1,
            (float)event->window.data2
        });
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appState, SDL_AppResult result)
{
    AppContext* context = (AppContext*)appState;

    if (context != NULL) {
        // Close Lua state.
        if (context->L) {
            lua_close(context->L);
        }

        // Release and clean up GPU resources.
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
