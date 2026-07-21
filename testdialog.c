#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------------------------------------------------------------------------
 * Callback invoked by SDL_ShowOpenFileDialog when user picks file(s) or cancels.
 * filelist is NULL on cancel, else NULL-terminated array of full paths.
 * filter_index indicates which filter was active (0-based).
 * --------------------------------------------------------------------------- */
static void SDLCALL on_file_dialog_result(void *userdata, const char * const *filelist, int filter_index)
{
    (void)filter_index;

    int *count = (int *)userdata;

    if (filelist == NULL) {
        SDL_Log("Dialog ERROR: %s", SDL_GetError());
        return;
    }
    if (filelist[0] == NULL) {
        SDL_Log("Dialog cancelled (user cancelled or no selection)");
        return;
    }

    for (int i = 0; filelist[i] != NULL; i++) {
        SDL_Log("[%d] %s", *count, filelist[i]);
        (*count)++;
    }
}

/* ---------------------------------------------------------------------------
 * Simple render-loop: draws background, prints instructions,
 * opens file dialog on mouse click.
 * --------------------------------------------------------------------------- */
int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("SDL_ShowOpenFileDialog Test", 640, 480, SDL_WINDOW_OPENGL);
    if (window == NULL) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
    if (renderer == NULL) {
        SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    /* ---- file filters (first is default) ---- */
    SDL_DialogFileFilter filters[] = {
        { .name = "Image files",  .pattern = "png;jpg;jpeg;gif;bmp" },
        { .name = "Text files",   .pattern = "txt;md;csv;json"        },
        { .name = "All files",    .pattern = "*"                      },
    };
    int num_filters = SDL_arraysize(filters);

    SDL_Log("SDL_ShowOpenFileDialog Test");
    SDL_Log("Left-click: open dialog with window attachment (sheet).");
    SDL_Log("Right-click: open dialog WITHOUT window (standalone modal).");
    SDL_Log("Middle-click: open dialog with window=NULL, allow_many=false.");
    SDL_Log("Close window or press ESC to quit.");

    int selection_count = 0;
    bool quit = false;

    while (!quit) {
        /* ---- process events ---- */
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_EVENT_QUIT:
                quit = true;
                break;

            case SDL_EVENT_KEY_DOWN:
                if (event.key.key == SDLK_ESCAPE) {
                    quit = true;
                }
                break;

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            {
                SDL_Window *target_win = window;
                bool allow_many = true;

                if (event.button.button == SDL_BUTTON_RIGHT) {
                    /* standalone modal — no window */
                    target_win = NULL;
                } else if (event.button.button == SDL_BUTTON_MIDDLE) {
                    target_win = NULL;
                    allow_many = false;
                }

                SDL_Log("Opening dialog (window=%p, allow_many=%d) ...",
                        (void *)target_win, allow_many);

                SDL_ShowOpenFileDialog(on_file_dialog_result,
                                        &selection_count,
                                        target_win,
                                        filters, num_filters,
                                        NULL,        /* default_location */
                                        allow_many);
                break;
            }

            default:
                break;
            }
        }

        /* ---- draw ---- */
        SDL_SetRenderDrawColor(renderer, 0x20, 0x20, 0x30, 0xFF);
        SDL_RenderClear(renderer);

        /* trivial instructions rendered as coloured rect + text (via debug overlay) */
        SDL_SetRenderDrawColor(renderer, 0x40, 0x60, 0x90, 0xFF);
        SDL_FRect rect = { 120.0f, 200.0f, 400.0f, 80.0f };
        SDL_RenderFillRect(renderer, &rect);

        SDL_RenderPresent(renderer);

        /* small delay to keep CPU cool */
        SDL_Delay(16);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    SDL_Log("Exiting. Total selections: %d", selection_count);
    return 0;
}
