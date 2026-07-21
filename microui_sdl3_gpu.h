/*
 * microui SDL3 GPU Backend
 *
 * Usage:
 *   1. mu_sdl3_gpu_init(device, window, render_format, &mu_ctx);
 *   2. In event loop: mu_sdl3_gpu_handle_event(event, mu_ctx);
 *   3. In render loop:
 *        mu_sdl3_gpu_render(cmd_buf, render_pass, mu_ctx);
 *   4. mu_sdl3_gpu_shutdown();
 */

#ifndef MU_SDL3_GPU_H
#define MU_SDL3_GPU_H

#include <stddef.h>
#include <stdio.h>
#include <string.h>
// #include <math.h>
#include "microui.h"
#include <SDL3/SDL.h>

/*================================================================================
 * Embedded 8x13 bitmap font (ASCII 32-126)
 * Public domain, originally from misc-fixed
 * Each glyph: 13 rows x 8 columns = 13 bytes, 1 bit per pixel (1=foreground)
 *================================================================================*/
#define MU_FONT_GLYPH_W 8
#define MU_FONT_GLYPH_H 13
#define MU_FONT_GLYPH_BYTES 13
#define MU_FONT_FIRST_CHAR 32
#define MU_FONT_NUM_CHARS 95
#define MU_FONT_GRID_COLS 16
#define MU_FONT_GRID_ROWS 6
#define MU_FONT_TEX_W (MU_FONT_GRID_COLS * MU_FONT_GLYPH_W) /* 128 */
#define MU_FONT_TEX_H (MU_FONT_GRID_ROWS * MU_FONT_GLYPH_H) /* 78  */

static const unsigned char mu_font_data[MU_FONT_NUM_CHARS][MU_FONT_GLYPH_BYTES] = {
    /* 32 ' ' */ {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* 33 '!' */
    {0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x00, 0x00},
    /* 34 '"' */
    {0x00, 0x00, 0x6c, 0x6c, 0x6c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* 35 '#' */
    {0x00, 0x00, 0x6c, 0x6c, 0xfe, 0x6c, 0x6c, 0xfe, 0x6c, 0x6c, 0x00, 0x00, 0x00},
    /* 36 '$' */
    {0x00, 0x18, 0x3c, 0x7e, 0x5a, 0x18, 0x3c, 0x7e, 0x5a, 0x18, 0x18, 0x00, 0x00},
    /* 37 '%' */
    {0x00, 0x00, 0x62, 0x64, 0x08, 0x10, 0x20, 0x4c, 0x8c, 0x00, 0x00, 0x00, 0x00},
    /* 38 '&' */
    {0x00, 0x00, 0x38, 0x6c, 0x6c, 0x38, 0x76, 0xdc, 0xcc, 0x76, 0x00, 0x00, 0x00},
    /* 39 ''' */
    {0x00, 0x00, 0x18, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* 40 '(' */
    {0x00, 0x00, 0x0c, 0x18, 0x30, 0x30, 0x30, 0x30, 0x30, 0x18, 0x0c, 0x00, 0x00},
    /* 41 ')' */
    {0x00, 0x00, 0x30, 0x18, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x18, 0x30, 0x00, 0x00},
    /* 42 '*' */
    {0x00, 0x00, 0x00, 0x66, 0x3c, 0xff, 0x3c, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* 43 '+' */
    {0x00, 0x00, 0x00, 0x18, 0x18, 0x7e, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* 44 ',' */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x30, 0x00},
    /* 45 '-' */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* 46 '.' */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00},
    /* 47 '/' */
    {0x00, 0x00, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x00, 0x00, 0x00, 0x00},
    /* 48 '0' */
    {0x00, 0x00, 0x3c, 0x66, 0x6e, 0x7e, 0x76, 0x66, 0x66, 0x3c, 0x00, 0x00, 0x00},
    /* 49 '1' */
    {0x00, 0x00, 0x18, 0x38, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3c, 0x00, 0x00, 0x00},
    /* 50 '2' */
    {0x00, 0x00, 0x3c, 0x66, 0x06, 0x0c, 0x18, 0x30, 0x60, 0x7e, 0x00, 0x00, 0x00},
    /* 51 '3' */
    {0x00, 0x00, 0x3c, 0x66, 0x06, 0x1c, 0x06, 0x06, 0x66, 0x3c, 0x00, 0x00, 0x00},
    /* 52 '4' */
    {0x00, 0x00, 0x0c, 0x1c, 0x3c, 0x6c, 0xcc, 0xfe, 0x0c, 0x0c, 0x00, 0x00, 0x00},
    /* 53 '5' */
    {0x00, 0x00, 0x7e, 0x60, 0x60, 0x7c, 0x06, 0x06, 0x66, 0x3c, 0x00, 0x00, 0x00},
    /* 54 '6' */
    {0x00, 0x00, 0x1c, 0x30, 0x60, 0x7c, 0x66, 0x66, 0x66, 0x3c, 0x00, 0x00, 0x00},
    /* 55 '7' */
    {0x00, 0x00, 0x7e, 0x06, 0x0c, 0x18, 0x30, 0x30, 0x30, 0x30, 0x00, 0x00, 0x00},
    /* 56 '8' */
    {0x00, 0x00, 0x3c, 0x66, 0x66, 0x3c, 0x66, 0x66, 0x66, 0x3c, 0x00, 0x00, 0x00},
    /* 57 '9' */
    {0x00, 0x00, 0x3c, 0x66, 0x66, 0x66, 0x3e, 0x06, 0x0c, 0x38, 0x00, 0x00, 0x00},
    /* 58 ':' */
    {0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00},
    /* 59 ';' */
    {0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x18, 0x18, 0x30, 0x00, 0x00},
    /* 60 '<' */
    {0x00, 0x00, 0x06, 0x0c, 0x18, 0x30, 0x60, 0x30, 0x18, 0x0c, 0x06, 0x00, 0x00},
    /* 61 '=' */
    {0x00, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* 62 '>' */
    {0x00, 0x00, 0x60, 0x30, 0x18, 0x0c, 0x06, 0x0c, 0x18, 0x30, 0x60, 0x00, 0x00},
    /* 63 '?' */
    {0x00, 0x00, 0x3c, 0x66, 0x06, 0x0c, 0x18, 0x18, 0x00, 0x18, 0x18, 0x00, 0x00},
    /* 64 '@' */
    {0x00, 0x00, 0x3c, 0x66, 0x6e, 0x7a, 0x72, 0x66, 0x60, 0x3c, 0x00, 0x00, 0x00},
    /* 65 'A' */
    {0x00, 0x00, 0x18, 0x3c, 0x66, 0x66, 0x7e, 0x66, 0x66, 0x66, 0x00, 0x00, 0x00},
    /* 66 'B' */
    {0x00, 0x00, 0x7c, 0x66, 0x66, 0x7c, 0x66, 0x66, 0x66, 0x7c, 0x00, 0x00, 0x00},
    /* 67 'C' */
    {0x00, 0x00, 0x3c, 0x66, 0x60, 0x60, 0x60, 0x60, 0x66, 0x3c, 0x00, 0x00, 0x00},
    /* 68 'D' */
    {0x00, 0x00, 0x78, 0x6c, 0x66, 0x66, 0x66, 0x66, 0x6c, 0x78, 0x00, 0x00, 0x00},
    /* 69 'E' */
    {0x00, 0x00, 0x7e, 0x60, 0x60, 0x7c, 0x60, 0x60, 0x60, 0x7e, 0x00, 0x00, 0x00},
    /* 70 'F' */
    {0x00, 0x00, 0x7e, 0x60, 0x60, 0x7c, 0x60, 0x60, 0x60, 0x60, 0x00, 0x00, 0x00},
    /* 71 'G' */
    {0x00, 0x00, 0x3c, 0x66, 0x60, 0x60, 0x6e, 0x66, 0x66, 0x3e, 0x00, 0x00, 0x00},
    /* 72 'H' */
    {0x00, 0x00, 0x66, 0x66, 0x66, 0x7e, 0x66, 0x66, 0x66, 0x66, 0x00, 0x00, 0x00},
    /* 73 'I' */
    {0x00, 0x00, 0x3c, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3c, 0x00, 0x00, 0x00},
    /* 74 'J' */
    {0x00, 0x00, 0x1e, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x6c, 0x38, 0x00, 0x00, 0x00},
    /* 75 'K' */
    {0x00, 0x00, 0x66, 0x6c, 0x78, 0x70, 0x78, 0x6c, 0x66, 0x66, 0x00, 0x00, 0x00},
    /* 76 'L' */
    {0x00, 0x00, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7e, 0x00, 0x00, 0x00},
    /* 77 'M' */
    {0x00, 0x00, 0x66, 0x66, 0x76, 0x7e, 0x6e, 0x66, 0x66, 0x66, 0x00, 0x00, 0x00},
    /* 78 'N' */
    {0x00, 0x00, 0x66, 0x66, 0x76, 0x7e, 0x6e, 0x66, 0x66, 0x66, 0x00, 0x00, 0x00},
    /* 79 'O' */
    {0x00, 0x00, 0x3c, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x00, 0x00, 0x00},
    /* 80 'P' */
    {0x00, 0x00, 0x7c, 0x66, 0x66, 0x66, 0x7c, 0x60, 0x60, 0x60, 0x00, 0x00, 0x00},
    /* 81 'Q' */
    {0x00, 0x00, 0x3c, 0x66, 0x66, 0x66, 0x66, 0x6e, 0x3c, 0x07, 0x00, 0x00, 0x00},
    /* 82 'R' */
    {0x00, 0x00, 0x7c, 0x66, 0x66, 0x7c, 0x78, 0x6c, 0x66, 0x66, 0x00, 0x00, 0x00},
    /* 83 'S' */
    {0x00, 0x00, 0x3c, 0x66, 0x60, 0x3c, 0x06, 0x06, 0x66, 0x3c, 0x00, 0x00, 0x00},
    /* 84 'T' */
    {0x00, 0x00, 0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x00, 0x00},
    /* 85 'U' */
    {0x00, 0x00, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x00, 0x00, 0x00},
    /* 86 'V' */
    {0x00, 0x00, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x3c, 0x18, 0x00, 0x00, 0x00},
    /* 87 'W' */
    {0x00, 0x00, 0x66, 0x66, 0x66, 0x6e, 0x7e, 0x76, 0x66, 0x66, 0x00, 0x00, 0x00},
    /* 88 'X' */
    {0x00, 0x00, 0x66, 0x66, 0x3c, 0x18, 0x3c, 0x66, 0x66, 0x66, 0x00, 0x00, 0x00},
    /* 89 'Y' */
    {0x00, 0x00, 0x66, 0x66, 0x66, 0x3c, 0x18, 0x18, 0x18, 0x18, 0x00, 0x00, 0x00},
    /* 90 'Z' */
    {0x00, 0x00, 0x7e, 0x06, 0x0c, 0x18, 0x30, 0x60, 0x60, 0x7e, 0x00, 0x00, 0x00},
    /* 91 '[' */
    {0x00, 0x00, 0x3c, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3c, 0x00, 0x00},
    /* 92 '\' */
    {0x00, 0x00, 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x00, 0x00, 0x00, 0x00},
    /* 93 ']' */
    {0x00, 0x00, 0x3c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x3c, 0x00, 0x00},
    /* 94 '^' */
    {0x00, 0x00, 0x18, 0x3c, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* 95 '_' */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00},
    /* 96 '`' */
    {0x00, 0x00, 0x18, 0x18, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /* 97 'a' */
    {0x00, 0x00, 0x00, 0x00, 0x3c, 0x06, 0x3e, 0x66, 0x66, 0x3e, 0x00, 0x00, 0x00},
    /* 98 'b' */
    {0x00, 0x00, 0x60, 0x60, 0x7c, 0x66, 0x66, 0x66, 0x66, 0x7c, 0x00, 0x00, 0x00},
    /* 99 'c' */
    {0x00, 0x00, 0x00, 0x00, 0x3c, 0x66, 0x60, 0x60, 0x66, 0x3c, 0x00, 0x00, 0x00},
    /* 100 'd' */
    {0x00, 0x00, 0x06, 0x06, 0x3e, 0x66, 0x66, 0x66, 0x66, 0x3e, 0x00, 0x00, 0x00},
    /* 101 'e' */
    {0x00, 0x00, 0x00, 0x00, 0x3c, 0x66, 0x7e, 0x60, 0x66, 0x3c, 0x00, 0x00, 0x00},
    /* 102 'f' */
    {0x00, 0x00, 0x1c, 0x36, 0x30, 0x78, 0x30, 0x30, 0x30, 0x30, 0x00, 0x00, 0x00},
    /* 103 'g' */
    {0x00, 0x00, 0x00, 0x00, 0x3e, 0x66, 0x66, 0x66, 0x66, 0x3e, 0x06, 0x66, 0x3c},
    /* 104 'h' */
    {0x00, 0x00, 0x60, 0x60, 0x7c, 0x66, 0x66, 0x66, 0x66, 0x66, 0x00, 0x00, 0x00},
    /* 105 'i' */
    {0x00, 0x00, 0x18, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x00, 0x00},
    /* 106 'j' */
    {0x00, 0x00, 0x0c, 0x00, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x6c, 0x38},
    /* 107 'k' */
    {0x00, 0x00, 0x60, 0x60, 0x66, 0x6c, 0x78, 0x78, 0x6c, 0x66, 0x00, 0x00, 0x00},
    /* 108 'l' */
    {0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x1c, 0x00, 0x00, 0x00},
    /* 109 'm' */
    {0x00, 0x00, 0x00, 0x00, 0x7c, 0x6e, 0x6e, 0x6e, 0x6e, 0x6e, 0x00, 0x00, 0x00},
    /* 110 'n' */
    {0x00, 0x00, 0x00, 0x00, 0x7c, 0x66, 0x66, 0x66, 0x66, 0x66, 0x00, 0x00, 0x00},
    /* 111 'o' */
    {0x00, 0x00, 0x00, 0x00, 0x3c, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x00, 0x00, 0x00},
    /* 112 'p' */
    {0x00, 0x00, 0x00, 0x00, 0x7c, 0x66, 0x66, 0x66, 0x66, 0x7c, 0x60, 0x60, 0x60},
    /* 113 'q' */
    {0x00, 0x00, 0x00, 0x00, 0x3e, 0x66, 0x66, 0x66, 0x66, 0x3e, 0x06, 0x06, 0x06},
    /* 114 'r' */
    {0x00, 0x00, 0x00, 0x00, 0x7c, 0x66, 0x60, 0x60, 0x60, 0x60, 0x00, 0x00, 0x00},
    /* 115 's' */
    {0x00, 0x00, 0x00, 0x00, 0x3e, 0x60, 0x3c, 0x06, 0x06, 0x7c, 0x00, 0x00, 0x00},
    /* 116 't' */
    {0x00, 0x00, 0x30, 0x30, 0x7c, 0x30, 0x30, 0x30, 0x30, 0x1c, 0x00, 0x00, 0x00},
    /* 117 'u' */
    {0x00, 0x00, 0x00, 0x00, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3e, 0x00, 0x00, 0x00},
    /* 118 'v' */
    {0x00, 0x00, 0x00, 0x00, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x18, 0x00, 0x00, 0x00},
    /* 119 'w' */
    {0x00, 0x00, 0x00, 0x00, 0x66, 0x6e, 0x6e, 0x7e, 0x3c, 0x18, 0x00, 0x00, 0x00},
    /* 120 'x' */
    {0x00, 0x00, 0x00, 0x00, 0x66, 0x66, 0x3c, 0x3c, 0x66, 0x66, 0x00, 0x00, 0x00},
    /* 121 'y' */
    {0x00, 0x00, 0x00, 0x00, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3e, 0x06, 0x66, 0x3c},
    /* 122 'z' */
    {0x00, 0x00, 0x00, 0x00, 0x7e, 0x0c, 0x18, 0x30, 0x60, 0x7e, 0x00, 0x00, 0x00},
    /* 123 '{' */
    {0x00, 0x00, 0x0e, 0x18, 0x18, 0x18, 0x70, 0x18, 0x18, 0x18, 0x0e, 0x00, 0x00},
    /* 124 '|' */
    {0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x00, 0x00},
    /* 125 '}' */
    {0x00, 0x00, 0x70, 0x18, 0x18, 0x18, 0x0e, 0x18, 0x18, 0x18, 0x70, 0x00, 0x00},
    /* 126 '~' */
    {0x00, 0x00, 0x72, 0x9c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
};

/*================================================================================
 * Icon shapes (rendered as simple geometry)
 *================================================================================*/
#define MU_ICON_TEX_W 32
#define MU_ICON_TEX_H 16
#define MU_ICON_SIZE 12

/*================================================================================
 * Vertex format (matches shader)
 *================================================================================*/
typedef struct
{
    float position[2];
    float uv[2];
    Uint8 color[4];
} MuVertex;

/*================================================================================
 * Device context
 *================================================================================*/
typedef struct MuSDL3GPU_Device
{
    SDL_GPUDevice *device;
    SDL_Window *window;

    SDL_GPUShader *vertex_shader;
    SDL_GPUShader *fragment_shader;
    SDL_GPUGraphicsPipeline *pipeline;

    SDL_GPUTexture *font_texture;
    SDL_GPUTexture *icon_texture;
    SDL_GPUSampler *sampler;
    SDL_GPUTexture *white_texture;

    /* dynamic quad buffer */
    SDL_GPUBuffer *vertex_buffer;
    SDL_GPUBuffer *index_buffer;
    Uint32 vertex_buffer_capacity;
    Uint32 index_buffer_capacity;
    Uint32 vertex_count;
    Uint32 index_count;

    MuVertex *vertex_data;
    Uint16 *index_data;
    Uint32 vertex_data_cap;
    Uint32 index_data_cap;
    int font_glyph_h;
    int font_glyph_w;
} MuSDL3GPU_Device;

static MuSDL3GPU_Device mu_gpu;

/*================================================================================
 * MSL Shaders (Metal Shading Language) — works on Apple Silicon
 *================================================================================*/
static const char *mu_msl_vert = "#include <metal_stdlib>\n"
                                 "using namespace metal;\n"
                                 "struct VertexIn {\n"
                                 "    float2 position [[attribute(0)]];\n"
                                 "    float2 uv [[attribute(1)]];\n"
                                 "    uchar4 color [[attribute(2)]];\n"
                                 "};\n"
                                 "struct VertexOut {\n"
                                 "    float4 position [[position]];\n"
                                 "    float2 uv;\n"
                                 "    float4 color;\n"
                                 "};\n"
                                 "struct Uniforms {\n"
                                 "    float4x4 projection;\n"
                                 "};\n"
                                 "vertex VertexOut main0(VertexIn in [[stage_in]], constant Uniforms "
                                 "&uniforms [[buffer(0)]]) {\n"
                                 "    VertexOut out;\n"
                                 "    out.position = uniforms.projection * float4(in.position, 0.0, 1.0);\n"
                                 "    out.uv = in.uv;\n"
                                 "    out.color = float4(in.color) / 255.0;\n"
                                 "    return out;\n"
                                 "}\n";

static const char *mu_msl_frag = "#include <metal_stdlib>\n"
                                 "using namespace metal;\n"
                                 "struct VertexOut {\n"
                                 "    float4 position [[position]];\n"
                                 "    float2 uv;\n"
                                 "    float4 color;\n"
                                 "};\n"
                                 "fragment float4 main0(VertexOut in [[stage_in]], texture2d<float> texture "
                                 "[[texture(0)]], sampler samplr [[sampler(0)]]) {\n"
                                 "    return in.color * texture.sample(samplr, in.uv);\n"
                                 "}\n";

/*================================================================================
 * Quad accumulation helpers
 *================================================================================*/
static void mu_ensure_vertex_cap(int needed)
{
    if (mu_gpu.vertex_data_cap < (Uint32)needed)
    {
        mu_gpu.vertex_data_cap = (Uint32)needed * 2;
        mu_gpu.vertex_data = SDL_realloc(mu_gpu.vertex_data, mu_gpu.vertex_data_cap * sizeof(MuVertex));
    }
}

static void mu_ensure_index_cap(int needed)
{
    if (mu_gpu.index_data_cap < (Uint32)needed)
    {
        mu_gpu.index_data_cap = (Uint32)needed * 2;
        mu_gpu.index_data = SDL_realloc(mu_gpu.index_data, mu_gpu.index_data_cap * sizeof(Uint16));
    }
}

static void mu_push_quad(float x1, float y1, float x2, float y2, float u1, float v1, float u2, float v2, Uint8 r,
                         Uint8 g, Uint8 b, Uint8 a)
{
    int vi = mu_gpu.vertex_count;
    int ii = mu_gpu.index_count;

    mu_ensure_vertex_cap(vi + 4);
    mu_ensure_index_cap(ii + 6);

    MuVertex *v = &mu_gpu.vertex_data[vi];
    v[0].position[0] = x1;
    v[0].position[1] = y1;
    v[0].uv[0] = u1;
    v[0].uv[1] = v1;
    v[0].color[0] = r;
    v[0].color[1] = g;
    v[0].color[2] = b;
    v[0].color[3] = a;

    v[1].position[0] = x2;
    v[1].position[1] = y1;
    v[1].uv[0] = u2;
    v[1].uv[1] = v1;
    v[1].color[0] = r;
    v[1].color[1] = g;
    v[1].color[2] = b;
    v[1].color[3] = a;

    v[2].position[0] = x2;
    v[2].position[1] = y2;
    v[2].uv[0] = u2;
    v[2].uv[1] = v2;
    v[2].color[0] = r;
    v[2].color[1] = g;
    v[2].color[2] = b;
    v[2].color[3] = a;

    v[3].position[0] = x1;
    v[3].position[1] = y2;
    v[3].uv[0] = u1;
    v[3].uv[1] = v2;
    v[3].color[0] = r;
    v[3].color[1] = g;
    v[3].color[2] = b;
    v[3].color[3] = a;

    mu_gpu.index_data[ii + 0] = vi + 0;
    mu_gpu.index_data[ii + 1] = vi + 1;
    mu_gpu.index_data[ii + 2] = vi + 2;
    mu_gpu.index_data[ii + 3] = vi + 0;
    mu_gpu.index_data[ii + 4] = vi + 2;
    mu_gpu.index_data[ii + 5] = vi + 3;

    mu_gpu.vertex_count += 4;
    mu_gpu.index_count += 6;
}

/*================================================================================
 * Font callbacks for microui
 *================================================================================*/
static int mu_text_width_cb(mu_Font font, const char *str, int len)
{
    (void)font;
    if (len < 0)
        len = (int)strlen(str);
    return len * MU_FONT_GLYPH_W;
}

static int mu_text_height_cb(mu_Font font)
{
    (void)font;
    return MU_FONT_GLYPH_H;
}

/*================================================================================
 * Setup: create GPU resources
 *================================================================================*/
static int mu_textures_uploaded = 0;

static void mu_upload_textures(SDL_GPUCommandBuffer *cmd)
{
    /* Build pixel data */
    int font_pitch = MU_FONT_TEX_W * 4;
    Uint8 *font_pixels = SDL_calloc(1, MU_FONT_TEX_H * font_pitch);
    for (int ci = 0; ci < MU_FONT_NUM_CHARS; ci++)
    {
        int gx = (ci % MU_FONT_GRID_COLS) * MU_FONT_GLYPH_W;
        int gy = (ci / MU_FONT_GRID_COLS) * MU_FONT_GLYPH_H;
        for (int row = 0; row < MU_FONT_GLYPH_H; row++)
        {
            unsigned char bits = mu_font_data[ci][row];
            for (int col = 0; col < MU_FONT_GLYPH_W; col++)
            {
                int on = (bits >> (7 - col)) & 1;
                Uint8 *p = &font_pixels[(gy + row) * font_pitch + (gx + col) * 4];
                p[0] = 255;
                p[1] = 255;
                p[2] = 255;
                p[3] = on ? 255 : 0;
            }
        }
    }

    int icon_pitch = MU_ICON_TEX_W * 4;
    Uint8 *icon_pixels = SDL_calloc(1, MU_ICON_TEX_H * icon_pitch);
    for (int r = 0; r < MU_ICON_SIZE; r++)
    {
        int px = 2 + r, py = 2 + r;
        icon_pixels[py * icon_pitch + px * 4 + 3] = 255;
        px = 2 + (MU_ICON_SIZE - 1 - r);
        icon_pixels[py * icon_pitch + px * 4 + 3] = 255;
    }
    int cx = 2 + MU_ICON_SIZE + 2, cy = 2;
    for (int y = 0; y < 4; y++)
    {
        int px = cx + 3 + y, py = cy + 7 + y;
        icon_pixels[py * icon_pitch + px * 4 + 3] = 255;
    }
    for (int y = 0; y <= 6; y++)
    {
        int px = cx + 5 + y, py = cy + 9 - y;
        icon_pixels[py * icon_pitch + px * 4 + 3] = 255;
    }
    cx = 2 + (MU_ICON_SIZE + 2) * 2;
    for (int y = 0; y < MU_ICON_SIZE; y++)
    {
        for (int x = 0; x < MU_ICON_SIZE - y / 2; x++)
        {
            icon_pixels[(cy + y) * icon_pitch + (cx + x) * 4 + 3] = 255;
        }
    }
    cx = 2 + (MU_ICON_SIZE + 2) * 3;
    for (int y = 0; y < MU_ICON_SIZE; y++)
    {
        for (int x = y / 2; x < MU_ICON_SIZE - y / 2; x++)
        {
            icon_pixels[(cy + y) * icon_pitch + (cx + x) * 4 + 3] = 255;
        }
    }

    Uint8 white_pixel[4] = {255, 255, 255, 255};

    /* Single transfer buffer for all textures */
    int font_size = MU_FONT_TEX_H * font_pitch;
    int icon_size = MU_ICON_TEX_H * icon_pitch;
    int total = font_size + icon_size + 4;

    SDL_GPUTransferBufferCreateInfo tb_info;
    SDL_zero(tb_info);
    tb_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tb_info.size = total;
    SDL_GPUTransferBuffer *tbuf = SDL_CreateGPUTransferBuffer(mu_gpu.device, &tb_info);
    Uint8 *map = SDL_MapGPUTransferBuffer(mu_gpu.device, tbuf, false);
    SDL_memcpy(map, font_pixels, font_size);
    SDL_memcpy(map + font_size, icon_pixels, icon_size);
    SDL_memcpy(map + font_size + icon_size, white_pixel, 4);
    SDL_UnmapGPUTransferBuffer(mu_gpu.device, tbuf);
    SDL_free(font_pixels);
    SDL_free(icon_pixels);

    /* Upload using caller's command buffer */
    SDL_GPUCopyPass *cp = SDL_BeginGPUCopyPass(cmd);

    SDL_GPUTextureTransferInfo src;
    SDL_GPUTextureRegion dst;

    SDL_zero(src);
    src.transfer_buffer = tbuf;
    src.offset = 0;
    src.pixels_per_row = MU_FONT_TEX_W;
    src.rows_per_layer = MU_FONT_TEX_H;
    SDL_zero(dst);
    dst.texture = mu_gpu.font_texture;
    dst.w = MU_FONT_TEX_W;
    dst.h = MU_FONT_TEX_H;
    dst.d = 1;
    SDL_UploadToGPUTexture(cp, &src, &dst, false);

    SDL_zero(src);
    src.transfer_buffer = tbuf;
    src.offset = font_size;
    src.pixels_per_row = MU_ICON_TEX_W;
    src.rows_per_layer = MU_ICON_TEX_H;
    SDL_zero(dst);
    dst.texture = mu_gpu.icon_texture;
    dst.w = MU_ICON_TEX_W;
    dst.h = MU_ICON_TEX_H;
    dst.d = 1;
    SDL_UploadToGPUTexture(cp, &src, &dst, false);

    SDL_zero(src);
    src.transfer_buffer = tbuf;
    src.offset = font_size + icon_size;
    SDL_zero(dst);
    dst.texture = mu_gpu.white_texture;
    dst.w = 1;
    dst.h = 1;
    dst.d = 1;
    SDL_UploadToGPUTexture(cp, &src, &dst, false);

    SDL_EndGPUCopyPass(cp);
    SDL_ReleaseGPUTransferBuffer(mu_gpu.device, tbuf);

    mu_textures_uploaded = 1;
}

static MuSDL3GPU_Device *mu_sdl3_gpu_device_create(void)
{
    SDL_GPUShaderCreateInfo shader_info;
    SDL_zero(shader_info);

    /* Vertex shader */
    shader_info.stage = SDL_GPU_SHADERSTAGE_VERTEX;
    shader_info.format = SDL_GPU_SHADERFORMAT_MSL;
    shader_info.code = (const Uint8 *)mu_msl_vert;
    shader_info.code_size = SDL_strlen(mu_msl_vert);
    shader_info.entrypoint = "main0";
    shader_info.num_uniform_buffers = 1;

    mu_gpu.vertex_shader = SDL_CreateGPUShader(mu_gpu.device, &shader_info);
    if (!mu_gpu.vertex_shader)
    {
        SDL_Log("Failed to create vertex shader: %s", SDL_GetError());
        return NULL;
    }

    /* Fragment shader */
    shader_info.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    shader_info.format = SDL_GPU_SHADERFORMAT_MSL;
    shader_info.code = (const Uint8 *)mu_msl_frag;
    shader_info.code_size = SDL_strlen(mu_msl_frag);
    shader_info.entrypoint = "main0";
    shader_info.num_samplers = 1;
    shader_info.num_uniform_buffers = 0;

    mu_gpu.fragment_shader = SDL_CreateGPUShader(mu_gpu.device, &shader_info);
    if (!mu_gpu.fragment_shader)
    {
        SDL_Log("Failed to create fragment shader: %s", SDL_GetError());
        return NULL;
    }

    /* Pipeline */
    SDL_GPUGraphicsPipelineCreateInfo pipeline_info;
    SDL_zero(pipeline_info);
    pipeline_info.vertex_shader = mu_gpu.vertex_shader;
    pipeline_info.fragment_shader = mu_gpu.fragment_shader;

    SDL_GPUVertexAttribute attributes[3];
    SDL_zero(attributes);
    attributes[0].location = 0;
    attributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    attributes[0].offset = offsetof(MuVertex, position);
    attributes[1].location = 1;
    attributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    attributes[1].offset = offsetof(MuVertex, uv);
    attributes[2].location = 2;
    attributes[2].format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4;
    attributes[2].offset = offsetof(MuVertex, color);

    SDL_GPUVertexBufferDescription binding;
    SDL_zero(binding);
    binding.slot = 0;
    binding.pitch = sizeof(MuVertex);
    binding.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

    pipeline_info.vertex_input_state.num_vertex_attributes = 3;
    pipeline_info.vertex_input_state.vertex_attributes = attributes;
    pipeline_info.vertex_input_state.num_vertex_buffers = 1;
    pipeline_info.vertex_input_state.vertex_buffer_descriptions = &binding;

    pipeline_info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
    pipeline_info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    pipeline_info.rasterizer_state.front_face = SDL_GPU_FRONTFACE_CLOCKWISE;

    SDL_GPUColorTargetDescription target_desc;
    SDL_zero(target_desc);
    target_desc.format = SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM;
    target_desc.blend_state.enable_blend = true;
    target_desc.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    target_desc.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    target_desc.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
    target_desc.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    target_desc.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    target_desc.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;

    pipeline_info.target_info.num_color_targets = 1;
    pipeline_info.target_info.color_target_descriptions = &target_desc;

    mu_gpu.pipeline = SDL_CreateGPUGraphicsPipeline(mu_gpu.device, &pipeline_info);
    if (!mu_gpu.pipeline)
    {
        SDL_Log("Failed to create pipeline: %s", SDL_GetError());
        return NULL;
    }

    /* Sampler */
    SDL_GPUSamplerCreateInfo sampler_info;
    SDL_zero(sampler_info);
    sampler_info.min_filter = SDL_GPU_FILTER_NEAREST;
    sampler_info.mag_filter = SDL_GPU_FILTER_NEAREST;
    sampler_info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    sampler_info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler_info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler_info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;

    mu_gpu.sampler = SDL_CreateGPUSampler(mu_gpu.device, &sampler_info);

    /* Allocate textures (upload deferred to first render) */
    {
        SDL_GPUTextureCreateInfo info;
        SDL_zero(info);
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        info.width = MU_FONT_TEX_W;
        info.height = MU_FONT_TEX_H;
        info.layer_count_or_depth = 1;
        info.num_levels = 1;
        info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        mu_gpu.font_texture = SDL_CreateGPUTexture(mu_gpu.device, &info);

        info.width = MU_ICON_TEX_W;
        info.height = MU_ICON_TEX_H;
        mu_gpu.icon_texture = SDL_CreateGPUTexture(mu_gpu.device, &info);

        info.width = 1;
        info.height = 1;
        mu_gpu.white_texture = SDL_CreateGPUTexture(mu_gpu.device, &info);
    }

    return &mu_gpu;
}

/*================================================================================
 * Public API
 *================================================================================*/

void mu_sdl3_gpu_init(SDL_GPUDevice *device, SDL_Window *window, mu_Context *ctx)
{
    SDL_zero(mu_gpu);
    mu_gpu.device = device;
    mu_gpu.window = window;

    mu_sdl3_gpu_device_create();

    mu_gpu.font_glyph_w = MU_FONT_GLYPH_W;
    mu_gpu.font_glyph_h = MU_FONT_GLYPH_H;

    mu_init(ctx);
    ctx->text_width = mu_text_width_cb;
    ctx->text_height = mu_text_height_cb;
    ctx->style->font = (mu_Font)1; /* non-null sentinel */
}

/* Call this before the render pass each frame, with the acquired command buffer
 */
void mu_sdl3_gpu_frame_start(SDL_GPUCommandBuffer *cmd_buf)
{
    if (!mu_textures_uploaded && mu_gpu.device)
    {
        mu_upload_textures(cmd_buf);
    }
}

void mu_sdl3_gpu_handle_event(SDL_Event *evt, mu_Context *ctx)
{
    switch (evt->type)
    {
    case SDL_EVENT_MOUSE_MOTION:
        mu_input_mousemove(ctx, (int)evt->motion.x, (int)evt->motion.y);
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN: {
        int btn = 0;
        if (evt->button.button == SDL_BUTTON_LEFT)
            btn = MU_MOUSE_LEFT;
        if (evt->button.button == SDL_BUTTON_RIGHT)
            btn = MU_MOUSE_RIGHT;
        if (evt->button.button == SDL_BUTTON_MIDDLE)
            btn = MU_MOUSE_MIDDLE;
        mu_input_mousedown(ctx, (int)evt->button.x, (int)evt->button.y, btn);
        break;
    }
    case SDL_EVENT_MOUSE_BUTTON_UP: {
        int btn = 0;
        if (evt->button.button == SDL_BUTTON_LEFT)
            btn = MU_MOUSE_LEFT;
        if (evt->button.button == SDL_BUTTON_RIGHT)
            btn = MU_MOUSE_RIGHT;
        if (evt->button.button == SDL_BUTTON_MIDDLE)
            btn = MU_MOUSE_MIDDLE;
        mu_input_mouseup(ctx, (int)evt->button.x, (int)evt->button.y, btn);
        break;
    }
    case SDL_EVENT_MOUSE_WHEEL:
        mu_input_scroll(ctx, (int)evt->wheel.x, (int)evt->wheel.y);
        break;
    case SDL_EVENT_KEY_DOWN:
        if (evt->key.key == SDLK_LSHIFT || evt->key.key == SDLK_RSHIFT)
            mu_input_keydown(ctx, MU_KEY_SHIFT);
        if (evt->key.key == SDLK_LCTRL || evt->key.key == SDLK_RCTRL)
            mu_input_keydown(ctx, MU_KEY_CTRL);
        if (evt->key.key == SDLK_LALT || evt->key.key == SDLK_RALT)
            mu_input_keydown(ctx, MU_KEY_ALT);
        if (evt->key.key == SDLK_BACKSPACE)
            mu_input_keydown(ctx, MU_KEY_BACKSPACE);
        if (evt->key.key == SDLK_RETURN)
            mu_input_keydown(ctx, MU_KEY_RETURN);
        break;
    case SDL_EVENT_KEY_UP:
        if (evt->key.key == SDLK_LSHIFT || evt->key.key == SDLK_RSHIFT)
            mu_input_keyup(ctx, MU_KEY_SHIFT);
        if (evt->key.key == SDLK_LCTRL || evt->key.key == SDLK_RCTRL)
            mu_input_keyup(ctx, MU_KEY_CTRL);
        if (evt->key.key == SDLK_LALT || evt->key.key == SDLK_RALT)
            mu_input_keyup(ctx, MU_KEY_ALT);
        if (evt->key.key == SDLK_BACKSPACE)
            mu_input_keyup(ctx, MU_KEY_BACKSPACE);
        if (evt->key.key == SDLK_RETURN)
            mu_input_keyup(ctx, MU_KEY_RETURN);
        break;
    case SDL_EVENT_TEXT_INPUT:
        mu_input_text(ctx, evt->text.text);
        break;
    }
}

/* Process mu commands and upload vertex data. Call BEFORE render pass. */
void mu_sdl3_gpu_upload(SDL_GPUCommandBuffer *cmd_buf, mu_Context *ctx)
{
    mu_Command *cmd = NULL;

    mu_gpu.vertex_count = 0;
    mu_gpu.index_count = 0;

    /* Iterate microui commands and build quads */
    while (mu_next_command(ctx, &cmd))
    {
        switch (cmd->type)
        {
        case MU_COMMAND_RECT: {
            mu_Rect r = cmd->rect.rect;
            mu_Color c = cmd->rect.color;
            mu_push_quad((float)r.x, (float)r.y, (float)(r.x + r.w), (float)(r.y + r.h), 0, 0, 0, 0, c.r, c.g, c.b,
                         c.a);
            break;
        }
        case MU_COMMAND_TEXT: {
            const char *str = cmd->text.str;
            int len = (int)strlen(str);
            int gx = cmd->text.pos.x;
            int gy = cmd->text.pos.y;
            mu_Color c = cmd->text.color;
            int gw = mu_gpu.font_glyph_w;
            int gh = mu_gpu.font_glyph_h;

            for (int i = 0; i < len; i++)
            {
                unsigned char ch = (unsigned char)str[i];
                if (ch < MU_FONT_FIRST_CHAR || ch > MU_FONT_FIRST_CHAR + MU_FONT_NUM_CHARS - 1)
                    continue;
                int idx = ch - MU_FONT_FIRST_CHAR;
                int tx = (idx % MU_FONT_GRID_COLS) * gw;
                int ty = (idx / MU_FONT_GRID_COLS) * gh;
                float u1 = (float)tx / MU_FONT_TEX_W;
                float v1 = (float)ty / MU_FONT_TEX_H;
                float u2 = (float)(tx + gw) / MU_FONT_TEX_W;
                float v2 = (float)(ty + gh) / MU_FONT_TEX_H;

                mu_push_quad((float)gx, (float)gy, (float)(gx + gw), (float)(gy + gh), u1, v1, u2, v2, c.r, c.g, c.b,
                             c.a);
                gx += gw;
            }
            break;
        }
        case MU_COMMAND_ICON: {
            int iid = cmd->icon.id;
            mu_Rect r = cmd->icon.rect;
            mu_Color c = cmd->icon.color;
            int icon_stride = MU_ICON_SIZE + 2;
            int tx = (iid - 1) * icon_stride + 2;
            int ty = 2;
            float u1 = (float)tx / MU_ICON_TEX_W;
            float v1 = (float)ty / MU_ICON_TEX_H;
            float u2 = (float)(tx + MU_ICON_SIZE) / MU_ICON_TEX_W;
            float v2 = (float)(ty + MU_ICON_SIZE) / MU_ICON_TEX_H;
            int xoff = (r.w - MU_ICON_SIZE) / 2;
            int yoff = (r.h - MU_ICON_SIZE) / 2;

            mu_push_quad((float)(r.x + xoff), (float)(r.y + yoff), (float)(r.x + xoff + MU_ICON_SIZE),
                         (float)(r.y + yoff + MU_ICON_SIZE), u1, v1, u2, v2, c.r, c.g, c.b, c.a);
            break;
        }
        default:
            break;
        }
    }

    if (mu_gpu.vertex_count == 0 || !mu_gpu.pipeline)
    {
        return;
    }

    /* Resize GPU buffers if needed */
    Uint32 vbytes = mu_gpu.vertex_count * sizeof(MuVertex);
    Uint32 ibytes = mu_gpu.index_count * sizeof(Uint16);

    if (mu_gpu.vertex_buffer_capacity < vbytes)
    {
        if (mu_gpu.vertex_buffer)
            SDL_ReleaseGPUBuffer(mu_gpu.device, mu_gpu.vertex_buffer);
        SDL_GPUBufferCreateInfo bi;
        SDL_zero(bi);
        bi.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
        bi.size = vbytes * 2;
        mu_gpu.vertex_buffer = SDL_CreateGPUBuffer(mu_gpu.device, &bi);
        mu_gpu.vertex_buffer_capacity = bi.size;
    }

    if (mu_gpu.index_buffer_capacity < ibytes)
    {
        if (mu_gpu.index_buffer)
            SDL_ReleaseGPUBuffer(mu_gpu.device, mu_gpu.index_buffer);
        SDL_GPUBufferCreateInfo bi;
        SDL_zero(bi);
        bi.usage = SDL_GPU_BUFFERUSAGE_INDEX;
        bi.size = ibytes * 2;
        mu_gpu.index_buffer = SDL_CreateGPUBuffer(mu_gpu.device, &bi);
        mu_gpu.index_buffer_capacity = bi.size;
    }

    /* Upload vertex/index data via copy pass */
    SDL_GPUTransferBufferCreateInfo tb_info;
    SDL_zero(tb_info);
    tb_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tb_info.size = vbytes + ibytes;

    SDL_GPUTransferBuffer *tbuf = SDL_CreateGPUTransferBuffer(mu_gpu.device, &tb_info);
    Uint8 *map = SDL_MapGPUTransferBuffer(mu_gpu.device, tbuf, false);
    SDL_memcpy(map, mu_gpu.vertex_data, vbytes);
    SDL_memcpy(map + vbytes, mu_gpu.index_data, ibytes);
    SDL_UnmapGPUTransferBuffer(mu_gpu.device, tbuf);

    SDL_GPUCopyPass *cp = SDL_BeginGPUCopyPass(cmd_buf);
    SDL_GPUTransferBufferLocation src;
    SDL_zero(src);
    src.transfer_buffer = tbuf;
    SDL_GPUBufferRegion dr;
    SDL_zero(dr);

    src.offset = 0;
    dr.buffer = mu_gpu.vertex_buffer;
    dr.offset = 0;
    dr.size = vbytes;
    SDL_UploadToGPUBuffer(cp, &src, &dr, false);

    src.offset = vbytes;
    dr.buffer = mu_gpu.index_buffer;
    dr.size = ibytes;
    SDL_UploadToGPUBuffer(cp, &src, &dr, false);
    SDL_EndGPUCopyPass(cp);
    SDL_ReleaseGPUTransferBuffer(mu_gpu.device, tbuf);
}

/* Draw already-uploaded vertex data. Call INSIDE render pass. */
void mu_sdl3_gpu_render(SDL_GPUCommandBuffer *cmd_buf, SDL_GPURenderPass *render_pass)
{
    if (!mu_gpu.pipeline || mu_gpu.vertex_count == 0)
    {
        return;
    }

    SDL_BindGPUGraphicsPipeline(render_pass, mu_gpu.pipeline);

    SDL_GPUBufferBinding vb;
    SDL_zero(vb);
    vb.buffer = mu_gpu.vertex_buffer;
    SDL_BindGPUVertexBuffers(render_pass, 0, &vb, 1);

    SDL_GPUBufferBinding ib;
    SDL_zero(ib);
    ib.buffer = mu_gpu.index_buffer;
    SDL_BindGPUIndexBuffer(render_pass, &ib, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    int w, h;
    SDL_GetWindowSizeInPixels(mu_gpu.window, &w, &h);
    SDL_GPUViewport vp = {0, 0, (float)w, (float)h, 0, 1};
    SDL_SetGPUViewport(render_pass, &vp);

    float proj[4][4] = {{2.0f / w, 0, 0, 0}, {0, -2.0f / h, 0, 0}, {0, 0, -1, 0}, {-1, 1, 0, 1}};
    SDL_PushGPUVertexUniformData(cmd_buf, 0, proj, sizeof(proj));

    SDL_GPUTextureSamplerBinding tex_binding;
    SDL_zero(tex_binding);
    tex_binding.texture = mu_gpu.font_texture;
    tex_binding.sampler = mu_gpu.sampler;
    SDL_BindGPUFragmentSamplers(render_pass, 0, &tex_binding, 1);

    SDL_DrawGPUIndexedPrimitives(render_pass, mu_gpu.index_count, 1, 0, 0, 0);
}

void mu_sdl3_gpu_shutdown(void)
{
    if (mu_gpu.vertex_buffer)
    {
        SDL_ReleaseGPUBuffer(mu_gpu.device, mu_gpu.vertex_buffer);
        mu_gpu.vertex_buffer = NULL;
    }
    if (mu_gpu.index_buffer)
    {
        SDL_ReleaseGPUBuffer(mu_gpu.device, mu_gpu.index_buffer);
        mu_gpu.index_buffer = NULL;
    }
    if (mu_gpu.font_texture)
    {
        SDL_ReleaseGPUTexture(mu_gpu.device, mu_gpu.font_texture);
        mu_gpu.font_texture = NULL;
    }
    if (mu_gpu.icon_texture)
    {
        SDL_ReleaseGPUTexture(mu_gpu.device, mu_gpu.icon_texture);
        mu_gpu.icon_texture = NULL;
    }
    if (mu_gpu.white_texture)
    {
        SDL_ReleaseGPUTexture(mu_gpu.device, mu_gpu.white_texture);
        mu_gpu.white_texture = NULL;
    }
    if (mu_gpu.sampler)
    {
        SDL_ReleaseGPUSampler(mu_gpu.device, mu_gpu.sampler);
        mu_gpu.sampler = NULL;
    }
    if (mu_gpu.pipeline)
    {
        SDL_ReleaseGPUGraphicsPipeline(mu_gpu.device, mu_gpu.pipeline);
        mu_gpu.pipeline = NULL;
    }
    if (mu_gpu.vertex_shader)
    {
        SDL_ReleaseGPUShader(mu_gpu.device, mu_gpu.vertex_shader);
        mu_gpu.vertex_shader = NULL;
    }
    if (mu_gpu.fragment_shader)
    {
        SDL_ReleaseGPUShader(mu_gpu.device, mu_gpu.fragment_shader);
        mu_gpu.fragment_shader = NULL;
    }

    if (mu_gpu.vertex_data)
    {
        SDL_free(mu_gpu.vertex_data);
        mu_gpu.vertex_data = NULL;
        mu_gpu.vertex_data_cap = 0;
    }
    if (mu_gpu.index_data)
    {
        SDL_free(mu_gpu.index_data);
        mu_gpu.index_data = NULL;
        mu_gpu.index_data_cap = 0;
    }

    SDL_zero(mu_gpu);
}

#endif /* MU_SDL3_GPU_H */
