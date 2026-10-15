#include "app.h"
#include "SDL3/SDL_dialog.h"
#include "SDL3/SDL_log.h"
#include "lua_script.h"
#include "model_import.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdlib.h>

#include "microui_sdl3_gpu.h"
#include "microui_ext.h"
#include <cgltf.h>

#define WINDOW_TITLE "Cumulus"
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

static const float CLEAR_COLOR[4] = {0.16f, 0.47f, 0.34f, 1.0f};

/* --- Widget-demo state (persists across frames) --- */
static struct
{
    int shape;        /* dropdown selection */
    mu_Real progress; /* progress bar / slider */
    int mode;         /* radio selection */
    mu_Color color;   /* color picker */
    int menu_open;    /* menu bar: open menu index, -1 = none */
    int last_action;  /* last menu item action (for display) */
} demo = {
    .shape = 0,
    .progress = 0.5f,
    .mode = 1,
    .color = {220, 120, 60, 255},
    .menu_open = -1,
    .last_action = -1,
};

/* File dialog filters — must stay alive until callback fires (SDL is async) */
static const SDL_DialogFileFilter FILE_FILTERS[] = {{.name = "glTF Model", .pattern = "gltf"},
                                                    {.name = "glTF Binary", .pattern = "glb"}};

/* Callback fired by SDL_ShowOpenFileDialog when user picks a file (or cancels)
 */
static void SDLCALL on_file_dialog_result(void *userdata, const char *const *files, int filter_index)
{
    (void)filter_index;
    AppContext *ctx = userdata;

    if (!files)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "File dialog error: %s", SDL_GetError());
        return;
    }

    if (!files[0])
    {
        SDL_Log("File dialog cancelled.");
        return;
    }

    SDL_Log("File selected: %s", files[0]);

    /* Free previous model if any */
    model_free(ctx->model);
    ctx->model = model_load(files[0]);
}

AppContext *app_init(void)
{
    SDL_SetAppMetadata(WINDOW_TITLE, "0.0.1", "com.arda.cumulus");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
        return NULL;
    }

    SDL_Window *window = SDL_CreateWindow(WINDOW_TITLE, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE);

    if (!window)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window: %s", SDL_GetError());
        return NULL;
    }

    /* We ship MSL (Metal) and SPIR-V (Vulkan) shaders for now. DXIL/D3D12 is
     * intentionally omitted so SDL picks Metal or Vulkan rather than selecting
     * a D3D12 device whose shaders we can't provide yet. */
    SDL_GPUShaderFormat shaderFormats =
        SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_MSL;

    SDL_GPUDevice *device = SDL_CreateGPUDevice(shaderFormats, false, NULL);
    if (!device)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create GPU device: %s", SDL_GetError());
        return NULL;
    }

    SDL_Log("Using %s GPU implementation.", SDL_GetGPUDeviceDriver(device));

    if (!SDL_ClaimWindowForGPUDevice(device, window))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_ClaimWindowForGPUDevice failed: %s", SDL_GetError());
        return NULL;
    }

    SDL_SetGPUSwapchainParameters(device, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC);

    AppContext *ctx = SDL_malloc(sizeof(AppContext));
    ctx->window = window;
    ctx->device = device;
    ctx->L = lua_script_init();
    ctx->model = NULL;
    mu_sdl3_gpu_init(device, window, &ctx->mu_ctx);

    return ctx;
}

static SDL_AppResult render_frame(AppContext *ctx, SDL_GPUCommandBuffer *cmdBuf)
{
    mu_sdl3_gpu_frame_start(cmdBuf);
    mu_sdl3_gpu_upload(cmdBuf, &ctx->mu_ctx);

    SDL_GPUTexture *swapchainTexture;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmdBuf, ctx->window, &swapchainTexture, NULL, NULL))
    {
        SDL_Log("SDL_WaitAndAcquireGPUSwapchainTexture failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (swapchainTexture)
    {
        SDL_GPUColorTargetInfo targetInfo = {
            .texture = swapchainTexture,
            .cycle = true,
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .store_op = SDL_GPU_STOREOP_STORE,
            .clear_color = {CLEAR_COLOR[0], CLEAR_COLOR[1], CLEAR_COLOR[2], CLEAR_COLOR[3]},
        };

        SDL_GPURenderPass *renderPass = SDL_BeginGPURenderPass(cmdBuf, &targetInfo, 1, NULL);
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
    if (mu_begin_window(&ctx->mu_ctx, "Cumulus", mu_rect(40, 40, 240, 140)))
    {
        mu_layout_row(&ctx->mu_ctx, 2, (int[]){80, -1}, 0);

        mu_label(&ctx->mu_ctx, "Model:");
        if (mu_button(&ctx->mu_ctx, "Load Model..."))
        {
            SDL_Log("Opening file dialog...\n");
            SDL_ShowOpenFileDialog(on_file_dialog_result, ctx, ctx->window, FILE_FILTERS, SDL_arraysize(FILE_FILTERS),NULL, false);
            SDL_Log("File dialog dispatched.\n");
        }

        if (ctx->model)
        {
            mu_label(&ctx->mu_ctx, "Loaded: yes");
        }

        mu_end_window(&ctx->mu_ctx);
    }

    /* Widget demo window */
    {
        static const char *shapes[] = {"Triangle", "Square", "Circle", "Pentagon"};

        static const mu_ext_MenuItem file_items[] = {
            {"New", 1}, {"Open...", 2}, {"Save", 3}, {"Quit", 4},
        };
        static const mu_ext_MenuItem edit_items[] = {
            {"Undo", 5}, {"Redo", 6},
        };
        static const mu_ext_Menu menus[] = {
            {"File", file_items, SDL_arraysize(file_items)},
            {"Edit", edit_items, SDL_arraysize(edit_items)},
        };

        if (mu_begin_window(&ctx->mu_ctx, "Widget Demo", mu_rect(320, 40, 300, 400)))
        {
            /* menu bar */
            int action = mu_ext_menu_bar(&ctx->mu_ctx, menus, SDL_arraysize(menus), &demo.menu_open);
            if (action >= 0)
            {
                demo.last_action = action;
            }

            /* dropdown */
            mu_layout_row(&ctx->mu_ctx, 2, (int[]){60, -1}, 0);
            mu_label(&ctx->mu_ctx, "Shape:");
            mu_ext_dropdown(&ctx->mu_ctx, "##shape", &demo.shape, shapes, SDL_arraysize(shapes));

            /* progress bar + tooltip */
            mu_layout_row(&ctx->mu_ctx, 2, (int[]){60, -1}, 0);
            mu_label(&ctx->mu_ctx, "Progress:");
            mu_ext_progress(&ctx->mu_ctx, demo.progress);
            mu_ext_tooltip(&ctx->mu_ctx, "Drag the slider below to change this");
            mu_layout_row(&ctx->mu_ctx, 2, (int[]){60, -1}, 0);
            mu_label(&ctx->mu_ctx, "Set:");
            mu_slider(&ctx->mu_ctx, &demo.progress, 0, 1);

            /* radio buttons */
            mu_layout_row(&ctx->mu_ctx, 1, (int[]){-1}, 0);
            mu_label(&ctx->mu_ctx, "Render mode:");
            mu_ext_radio(&ctx->mu_ctx, "Wireframe", &demo.mode, 0);
            mu_ext_radio(&ctx->mu_ctx, "Solid", &demo.mode, 1);
            mu_ext_radio(&ctx->mu_ctx, "Shaded", &demo.mode, 2);

            /* color picker */
            mu_layout_row(&ctx->mu_ctx, 2, (int[]){60, 40}, 20);
            mu_label(&ctx->mu_ctx, "Color:");
            mu_ext_color_picker(&ctx->mu_ctx, "##color", &demo.color);

            /* last menu action */
            if (demo.last_action >= 0)
            {
                char buf[64];
                SDL_snprintf(buf, sizeof(buf), "Last menu action: %d", demo.last_action);
                mu_layout_row(&ctx->mu_ctx, 1, (int[]){-1}, 0);
                mu_label(&ctx->mu_ctx, buf);
            }

            mu_end_window(&ctx->mu_ctx);
        }
    }
    mu_end(&ctx->mu_ctx);

    /* Render */
    SDL_GPUCommandBuffer *cmdBuf = SDL_AcquireGPUCommandBuffer(ctx->device);
    if (!cmdBuf)
    {
        SDL_Log("SDL_AcquireGPUCommandBuffer failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    return render_frame(ctx, cmdBuf);
}

SDL_AppResult app_event(AppContext *ctx, SDL_Event *event)
{
    if (event->type == SDL_EVENT_QUIT)
    {
        return SDL_APP_SUCCESS;
    }

    if (event->type == SDL_EVENT_KEY_DOWN)
    {
        if (event->key.key == SDLK_ESCAPE)
        {
            return SDL_APP_SUCCESS;
        }
        if (event->key.key == SDLK_R)
        {
            lua_script_reload(ctx->L);
        }
    }

    mu_sdl3_gpu_handle_event(event, &ctx->mu_ctx);
    return SDL_APP_CONTINUE;
}

void app_quit(AppContext *ctx)
{

    if (!ctx)
    {
        SDL_Quit();
        return;
    }

    model_free(ctx->model);
    mu_sdl3_gpu_shutdown();
    lua_script_shutdown(ctx->L);

    if (ctx->device)
    {
        if (ctx->window)
        {
            SDL_ReleaseWindowFromGPUDevice(ctx->device, ctx->window);
            SDL_DestroyWindow(ctx->window);
        }
        SDL_DestroyGPUDevice(ctx->device);
    }

    SDL_free(ctx);
    SDL_Quit();
}
