#include "app.h"
#include "lua_script.h"

#include <stdlib.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "microui_sdl3_gpu.h"

#define WINDOW_TITLE   "Cumulus"
#define WINDOW_WIDTH   800
#define WINDOW_HEIGHT  600

static const float CLEAR_COLOR[4] = { 0.16f, 0.47f, 0.34f, 1.0f };

AppContext* app_init(void)
{
    SDL_SetAppMetadata(WINDOW_TITLE, "0.0.1",
        "com.arda.cumulus");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
            "Couldn't initialize SDL: %s", SDL_GetError());
        return NULL;
    }

    SDL_Window *window = SDL_CreateWindow(
        WINDOW_TITLE, WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_RESIZABLE);

    if (!window) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
            "Couldn't create window: %s", SDL_GetError());
        return NULL;
    }

    SDL_GPUShaderFormat shaderFormats =
        SDL_GPU_SHADERFORMAT_SPIRV |
        SDL_GPU_SHADERFORMAT_DXIL |
        SDL_GPU_SHADERFORMAT_MSL;

    SDL_GPUDevice *device = SDL_CreateGPUDevice(
        shaderFormats, false, NULL);
    if (!device) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
            "Couldn't create GPU device: %s", SDL_GetError());
        return NULL;
    }

    SDL_Log("Using %s GPU implementation.",
        SDL_GetGPUDeviceDriver(device));

    if (!SDL_ClaimWindowForGPUDevice(device, window)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
            "SDL_ClaimWindowForGPUDevice failed: %s", SDL_GetError());
        return NULL;
    }

    SDL_SetGPUSwapchainParameters(device, window,
        SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
        SDL_GPU_PRESENTMODE_VSYNC);

    AppContext *ctx = SDL_malloc(sizeof(AppContext));
    ctx->window = window;
    ctx->device = device;
    ctx->L = lua_script_init();
    mu_sdl3_gpu_init(device, window, &ctx->mu_ctx);

    return ctx;
}

static SDL_AppResult render_frame(AppContext *ctx,
    SDL_GPUCommandBuffer *cmdBuf)
{
    mu_sdl3_gpu_frame_start(cmdBuf);
    mu_sdl3_gpu_upload(cmdBuf, &ctx->mu_ctx);

    SDL_GPUTexture *swapchainTexture;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmdBuf,
            ctx->window, &swapchainTexture, NULL, NULL)) {
        SDL_Log("SDL_WaitAndAcquireGPUSwapchainTexture failed: %s",
            SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (swapchainTexture) {
        SDL_GPUColorTargetInfo targetInfo = {
            .texture     = swapchainTexture,
            .cycle       = true,
            .load_op     = SDL_GPU_LOADOP_CLEAR,
            .store_op    = SDL_GPU_STOREOP_STORE,
            .clear_color = { CLEAR_COLOR[0], CLEAR_COLOR[1],
                             CLEAR_COLOR[2], CLEAR_COLOR[3] },
        };

        SDL_GPURenderPass *renderPass =
            SDL_BeginGPURenderPass(cmdBuf, &targetInfo, 1, NULL);
        mu_sdl3_gpu_render(cmdBuf, renderPass);
        SDL_EndGPURenderPass(renderPass);
    }

    SDL_SubmitGPUCommandBuffer(cmdBuf);
    return SDL_APP_CONTINUE;
}

SDL_AppResult app_iterate(AppContext *ctx)
{
    lua_script_update(ctx->L);

    /* Build microui UI */
    mu_begin(&ctx->mu_ctx);
    if (mu_begin_window(&ctx->mu_ctx, "Cumulus",
            mu_rect(40, 40, 300, 400))) {
        mu_layout_row(&ctx->mu_ctx, 2, (int[]) { 80, -1 }, 0);
        mu_label(&ctx->mu_ctx, "Hello, microui!");
        if (mu_button(&ctx->mu_ctx, "Button")) {
            SDL_Log("Button clicked!");
        }
        mu_end_window(&ctx->mu_ctx);
    }
    mu_end(&ctx->mu_ctx);

    /* Render */
    SDL_GPUCommandBuffer *cmdBuf =
        SDL_AcquireGPUCommandBuffer(ctx->device);
    if (!cmdBuf) {
        SDL_Log("SDL_AcquireGPUCommandBuffer failed: %s",
            SDL_GetError());
        return SDL_APP_FAILURE;
    }

    return render_frame(ctx, cmdBuf);
}

SDL_AppResult app_event(AppContext *ctx, SDL_Event *event)
{
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }

    if (event->type == SDL_EVENT_KEY_DOWN) {
        if (event->key.key == SDLK_ESCAPE) {
            return SDL_APP_SUCCESS;
        }
        if (event->key.key == SDLK_R) {
            lua_script_reload(ctx->L);
        }
    }

    mu_sdl3_gpu_handle_event(event, &ctx->mu_ctx);
    return SDL_APP_CONTINUE;
}

void app_quit(AppContext *ctx)
{
    if (!ctx) {
        SDL_Quit();
        return;
    }

    mu_sdl3_gpu_shutdown();
    lua_script_shutdown(ctx->L);

    if (ctx->device) {
        if (ctx->window) {
            SDL_ReleaseWindowFromGPUDevice(ctx->device, ctx->window);
            SDL_DestroyWindow(ctx->window);
        }
        SDL_DestroyGPUDevice(ctx->device);
    }

    SDL_free(ctx);
    SDL_Quit();
}
