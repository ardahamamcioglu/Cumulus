#include <stddef.h>
#include <SDL3/SDL.h>
#include "nuklear.h"

struct NkSDL3GPU_Device;
typedef struct NkSDL3GPU_Device NkSDL3GPU_Device;

struct nk_draw_vertex;

NkSDL3GPU_Device* nk_sdl3_gpu_init(SDL_GPUDevice *device, SDL_Window *window, SDL_GPUTextureFormat render_format);
void nk_sdl3_gpu_font_stash_begin(NkSDL3GPU_Device *sdl3_gpu, struct nk_font_atlas **atlas);
void nk_sdl3_gpu_font_stash_end(NkSDL3GPU_Device *sdl3_gpu);
int nk_sdl3_gpu_handle_event(NkSDL3GPU_Device *sdl3_gpu, SDL_Event *evt);
void nk_sdl3_gpu_render_upload(NkSDL3GPU_Device *sdl3_gpu, SDL_GPUCopyPass* copy_pass);
void nk_sdl3_gpu_render_draw(NkSDL3GPU_Device *sdl3_gpu, SDL_GPUCommandBuffer *cmd, SDL_GPURenderPass *pass);
void nk_sdl3_gpu_shutdown(NkSDL3GPU_Device *sdl3_gpu);

#ifdef NK_SDL3_GPU_IMPLEMENTATION
struct NkSDL3GPU_Device {
    SDL_GPUDevice* device;
    SDL_Window* window;
    SDL_GPUTextureFormat render_format;

    SDL_GPUShader* vertex_shader;
    SDL_GPUShader* fragment_shader;
    SDL_GPUGraphicsPipeline* pipeline;

    SDL_GPUTexture* font_texture;
    SDL_GPUSampler* sampler;

    SDL_GPUBuffer* vertex_buffer;
    SDL_GPUBuffer* index_buffer;
    Uint32 vertex_buffer_size;
    Uint32 index_buffer_size;

    struct nk_context ctx;
    struct nk_font_atlas atlas;
    struct nk_buffer cmds;
    struct nk_draw_null_texture null;

    float width;
    float height;
    float display_width;
    float display_height;
};

struct nk_draw_vertex {
    float position[2];
    float uv[2];
    nk_byte col[4];
};

static Uint8* nk_sdl3_gpu_load_shader(const char *path, size_t *bytes_read) {
	Uint8* code = (Uint8*) SDL_LoadFile(path, bytes_read);
	if (code == NULL) {
		SDL_Log("Failed to load shader from disk! %s", path);
		return NULL;
	}
	return code;
}

static void nk_sdl3_gpu_device_create(NkSDL3GPU_Device *sdl3_gpu) {
    SDL_GPUShaderCreateInfo shader_info;
    SDL_zero(shader_info);
    
    /* Compile Vertex Shader */
    Uint8* vertex_shader_code = nk_sdl3_gpu_load_shader("./shader/nk_shader.vert.spv", &shader_info.code_size);
    shader_info.stage = SDL_GPU_SHADERSTAGE_VERTEX;
    shader_info.format = SDL_GPU_SHADERFORMAT_SPIRV;
    shader_info.code = vertex_shader_code;
    shader_info.entrypoint = "main";
    shader_info.num_uniform_buffers = 1;
    
    sdl3_gpu->vertex_shader = SDL_CreateGPUShader(sdl3_gpu->device, &shader_info);
    if (!sdl3_gpu->vertex_shader) {
        SDL_Log("Failed to create vertex shader: %s\n", SDL_GetError());
    }

    /* Compile Fragment Shader */
    Uint8* fragment_shader_code = nk_sdl3_gpu_load_shader("./shader/nk_shader.frag.spv", &shader_info.code_size);
    shader_info.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    shader_info.format = SDL_GPU_SHADERFORMAT_SPIRV;
    shader_info.code = fragment_shader_code;
    shader_info.entrypoint = "main";
    shader_info.num_samplers = 1;
    shader_info.num_uniform_buffers = 0;
    
    sdl3_gpu->fragment_shader = SDL_CreateGPUShader(sdl3_gpu->device, &shader_info);
    if (!sdl3_gpu->fragment_shader) {
        SDL_Log("Failed to create fragment shader: %s\n", SDL_GetError());
    }

    /* Create Pipeline */
    SDL_GPUGraphicsPipelineCreateInfo pipeline_info;
    SDL_zero(pipeline_info);
    
    pipeline_info.vertex_shader = sdl3_gpu->vertex_shader;
    pipeline_info.fragment_shader = sdl3_gpu->fragment_shader;
    
    /* Vertex Input State */
    SDL_GPUVertexAttribute attributes[3];
    SDL_zero(attributes);
    
    /* Position */
    attributes[0].location = 0;
    attributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    attributes[0].offset = offsetof(struct nk_draw_vertex, position);
    
    /* UV */
    attributes[1].location = 1;
    attributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    attributes[1].offset = offsetof(struct nk_draw_vertex, uv);
    
    /* Color */
    attributes[2].location = 2;
    attributes[2].format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4;
    attributes[2].offset = offsetof(struct nk_draw_vertex, col);
    
    SDL_GPUVertexBufferDescription binding;
    SDL_zero(binding);
    binding.slot = 0;
    binding.pitch = sizeof(struct nk_draw_vertex);
    binding.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    
    pipeline_info.vertex_input_state.num_vertex_attributes = 3;
    pipeline_info.vertex_input_state.vertex_attributes = attributes;
    pipeline_info.vertex_input_state.num_vertex_buffers = 1;
    pipeline_info.vertex_input_state.vertex_buffer_descriptions = &binding;
    
    /* Rasterizer State */
    pipeline_info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
    pipeline_info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    pipeline_info.rasterizer_state.front_face = SDL_GPU_FRONTFACE_CLOCKWISE;
    
    /* Blend State */
    SDL_GPUColorTargetDescription target_desc;
    SDL_zero(target_desc);
    target_desc.format = sdl3_gpu->render_format;
    target_desc.blend_state.enable_blend = true;
    target_desc.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    target_desc.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    target_desc.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
    target_desc.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    target_desc.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    target_desc.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
    
    pipeline_info.target_info.num_color_targets = 1;
    pipeline_info.target_info.color_target_descriptions = &target_desc;
    pipeline_info.target_info.has_depth_stencil_target = false;
    
    sdl3_gpu->pipeline = SDL_CreateGPUGraphicsPipeline(sdl3_gpu->device, &pipeline_info);
    if (!sdl3_gpu->pipeline) {
        SDL_Log("Failed to create pipeline: %s", SDL_GetError());
    }
    
    /* Create Sampler */
    SDL_GPUSamplerCreateInfo sampler_info;
    SDL_zero(sampler_info);
    sampler_info.min_filter = SDL_GPU_FILTER_LINEAR;
    sampler_info.mag_filter = SDL_GPU_FILTER_LINEAR;
    sampler_info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    sampler_info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    sampler_info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    sampler_info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    
    sdl3_gpu->sampler = SDL_CreateGPUSampler(sdl3_gpu->device, &sampler_info);
}

NkSDL3GPU_Device* nk_sdl3_gpu_init(SDL_GPUDevice *device, SDL_Window *window, SDL_GPUTextureFormat render_format) {
    NkSDL3GPU_Device* sdl3_gpu = (NkSDL3GPU_Device*) SDL_malloc(sizeof(NkSDL3GPU_Device));
    sdl3_gpu->device = device;
    sdl3_gpu->window = window;
    sdl3_gpu->render_format = render_format;
    
    nk_init_default(&sdl3_gpu->ctx, 0);
    nk_buffer_init_default(&sdl3_gpu->cmds);
    nk_sdl3_gpu_device_create(sdl3_gpu);
    return sdl3_gpu;
}

void nk_sdl3_gpu_font_stash_begin(NkSDL3GPU_Device *sdl3_gpu, struct nk_font_atlas **atlas) {
    nk_font_atlas_init_default(&sdl3_gpu->atlas);
    nk_font_atlas_begin(&sdl3_gpu->atlas);
    if (atlas) *atlas = &sdl3_gpu->atlas;
}

void nk_sdl3_gpu_font_stash_end(NkSDL3GPU_Device *sdl3_gpu) {
    const void *image;
    int w, h;
    image = nk_font_atlas_bake(&sdl3_gpu->atlas, &w, &h, NK_FONT_ATLAS_RGBA32);
    
    /* Upload font texture */
    SDL_GPUTextureCreateInfo texture_info;
    SDL_zero(texture_info);
    texture_info.type = SDL_GPU_TEXTURETYPE_2D;
    texture_info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    texture_info.width = w;
    texture_info.height = h;
    texture_info.layer_count_or_depth = 1;
    texture_info.num_levels = 1;
    texture_info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    
    sdl3_gpu->font_texture = SDL_CreateGPUTexture(sdl3_gpu->device, &texture_info);
    
    SDL_GPUTransferBufferCreateInfo transfer_info;
    SDL_zero(transfer_info);
    transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer_info.size = w * h * 4;
    
    SDL_GPUTransferBuffer *transfer_buffer = SDL_CreateGPUTransferBuffer(sdl3_gpu->device, &transfer_info);
    Uint8 *map = SDL_MapGPUTransferBuffer(sdl3_gpu->device, transfer_buffer, false);
    SDL_memcpy(map, image, w * h * 4);
    SDL_UnmapGPUTransferBuffer(sdl3_gpu->device, transfer_buffer);
    
    SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(sdl3_gpu->device);
    SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(cmd);
    
    SDL_GPUTextureTransferInfo source;
    SDL_zero(source);
    source.transfer_buffer = transfer_buffer;
    source.offset = 0;
    source.pixels_per_row = w;
    source.rows_per_layer = h;
    
    SDL_GPUTextureRegion destination;
    SDL_zero(destination);
    destination.texture = sdl3_gpu->font_texture;
    destination.w = w;
    destination.h = h;
    destination.d = 1;
    
    SDL_UploadToGPUTexture(copy_pass, &source, &destination, false);
    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(sdl3_gpu->device, transfer_buffer);
    
    nk_font_atlas_end(&sdl3_gpu->atlas, nk_handle_ptr(sdl3_gpu->font_texture), &sdl3_gpu->null);
    if (sdl3_gpu->atlas.default_font)
        nk_style_set_font(&sdl3_gpu->ctx, &sdl3_gpu->atlas.default_font->handle);
}

int nk_sdl3_gpu_handle_event(NkSDL3GPU_Device *sdl3_gpu, SDL_Event *evt) {
    struct nk_context *ctx = &sdl3_gpu->ctx;
    if (evt->type == SDL_EVENT_MOUSE_BUTTON_DOWN || evt->type == SDL_EVENT_MOUSE_BUTTON_UP) {
        int down = evt->type == SDL_EVENT_MOUSE_BUTTON_DOWN;
        const float x = evt->button.x, y = evt->button.y;
        if (evt->button.button == SDL_BUTTON_LEFT) {
            if (evt->button.clicks > 1)
                nk_input_button(ctx, NK_BUTTON_DOUBLE, (int)x, (int)y, down);
            nk_input_button(ctx, NK_BUTTON_LEFT, (int)x, (int)y, down);
        } else if (evt->button.button == SDL_BUTTON_MIDDLE)
            nk_input_button(ctx, NK_BUTTON_MIDDLE, (int)x, (int)y, down);
        else if (evt->button.button == SDL_BUTTON_RIGHT)
            nk_input_button(ctx, NK_BUTTON_RIGHT, (int)x, (int)y, down);
        return 1;
    } else if (evt->type == SDL_EVENT_MOUSE_MOTION) {
        if (ctx->input.mouse.grabbed) {
            int x = (int)ctx->input.mouse.prev.x, y = (int)ctx->input.mouse.prev.y;
            nk_input_motion(ctx, x + (int)evt->motion.xrel, y + (int)evt->motion.yrel);
        } else nk_input_motion(ctx, (int)evt->motion.x, (int)evt->motion.y);
        return 1;
    } else if (evt->type == SDL_EVENT_TEXT_INPUT) {
        nk_glyph glyph;
        memcpy(glyph, evt->text.text, NK_UTF_SIZE);
        nk_input_glyph(ctx, glyph);
        return 1;
    } else if (evt->type == SDL_EVENT_KEY_DOWN || evt->type == SDL_EVENT_KEY_UP) {
        int down = evt->type == SDL_EVENT_KEY_DOWN;
        if (evt->key.key == SDLK_RSHIFT || evt->key.key == SDLK_LSHIFT)
            nk_input_key(ctx, NK_KEY_SHIFT, down);
        else if (evt->key.key == SDLK_DELETE)
            nk_input_key(ctx, NK_KEY_DEL, down);
        else if (evt->key.key == SDLK_RETURN)
            nk_input_key(ctx, NK_KEY_ENTER, down);
        else if (evt->key.key == SDLK_TAB)
            nk_input_key(ctx, NK_KEY_TAB, down);
        else if (evt->key.key == SDLK_BACKSPACE)
            nk_input_key(ctx, NK_KEY_BACKSPACE, down);
        else if (evt->key.key == SDLK_HOME) {
            nk_input_key(ctx, NK_KEY_TEXT_START, down);
            nk_input_key(ctx, NK_KEY_SCROLL_START, down);
        } else if (evt->key.key == SDLK_END) {
            nk_input_key(ctx, NK_KEY_TEXT_END, down);
            nk_input_key(ctx, NK_KEY_SCROLL_END, down);
        } else if (evt->key.key == SDLK_PAGEDOWN)
            nk_input_key(ctx, NK_KEY_SCROLL_DOWN, down);
        else if (evt->key.key == SDLK_PAGEUP)
            nk_input_key(ctx, NK_KEY_SCROLL_UP, down);
        else if (evt->key.key == SDLK_C) {
            if (evt->key.mod & SDL_KMOD_CTRL)
                nk_input_key(ctx, NK_KEY_COPY, down);
        } else if (evt->key.key == SDLK_V) {
            if (evt->key.mod & SDL_KMOD_CTRL)
                nk_input_key(ctx, NK_KEY_PASTE, down);
        } else if (evt->key.key == SDLK_X) {
            if (evt->key.mod & SDL_KMOD_CTRL)
                nk_input_key(ctx, NK_KEY_CUT, down);
        } else if (evt->key.key == SDLK_Z) {
            if (evt->key.mod & SDL_KMOD_CTRL)
                nk_input_key(ctx, NK_KEY_TEXT_UNDO, down);
        } else if (evt->key.key == SDLK_LEFT) {
            if (evt->key.mod & SDL_KMOD_CTRL)
                nk_input_key(ctx, NK_KEY_TEXT_WORD_LEFT, down);
            else nk_input_key(ctx, NK_KEY_LEFT, down);
        } else if (evt->key.key == SDLK_RIGHT) {
            if (evt->key.mod & SDL_KMOD_CTRL)
                nk_input_key(ctx, NK_KEY_TEXT_WORD_RIGHT, down);
            else nk_input_key(ctx, NK_KEY_RIGHT, down);
        }
        return 1;
    } else if (evt->type == SDL_EVENT_MOUSE_WHEEL) {
        nk_input_scroll(ctx, nk_vec2(evt->wheel.x, evt->wheel.y));
        return 1;
    }
    return 0;
}

void nk_sdl3_gpu_render_upload(NkSDL3GPU_Device *sdl3_gpu, SDL_GPUCopyPass* copy_pass) {
    struct nk_context *ctx = &sdl3_gpu->ctx;
    
    /* Convert */
    static const struct nk_draw_vertex_layout_element vertex_layout[] = {
        {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, offsetof(struct nk_draw_vertex, position)},
        {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, offsetof(struct nk_draw_vertex, uv)},
        {NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, offsetof(struct nk_draw_vertex, col)},
        {NK_VERTEX_LAYOUT_END}
    };
    
    static struct nk_convert_config config;
    static int config_init = 0;
    if (!config_init) {
        SDL_memset(&config, 0, sizeof(config));
        config.vertex_layout = vertex_layout;
        config.vertex_size = sizeof(struct nk_draw_vertex);
        config.vertex_alignment = NK_ALIGNOF(struct nk_draw_vertex);
        config.circle_segment_count = 22;
        config.curve_segment_count = 22;
        config.arc_segment_count = 22;
        config.global_alpha = 1.0f;
        config.shape_AA = NK_ANTI_ALIASING_ON;
        config.line_AA = NK_ANTI_ALIASING_ON;
        config_init = 1;
    }
    
    #define MAX_VERTEX_MEMORY (512 * 1024)
    #define MAX_ELEMENT_MEMORY (128 * 1024)
    
    static void *vertex_data = NULL;
    static void *element_data = NULL;
    
    if (!vertex_data) vertex_data = SDL_malloc(MAX_VERTEX_MEMORY);
    if (!element_data) element_data = SDL_malloc(MAX_ELEMENT_MEMORY);
    
    struct nk_buffer vbuf, ebuf;
    nk_buffer_init_fixed(&vbuf, vertex_data, MAX_VERTEX_MEMORY);
    nk_buffer_init_fixed(&ebuf, element_data, MAX_ELEMENT_MEMORY);
    
    nk_convert(ctx, &sdl3_gpu->cmds, &vbuf, &ebuf, &config);
    
    Uint32 v_size = (Uint32)vbuf.needed;
    Uint32 e_size = (Uint32)ebuf.needed;
    
    if (v_size == 0 || e_size == 0) {
        return;
    }
    
    /* Resize buffers */
    if (sdl3_gpu->vertex_buffer_size < v_size) {
        // THIS LINE WAS CAUSING A CRASH (0xC0000005)
        if (sdl3_gpu->vertex_buffer) {
            SDL_Log("\n\nReleasing nuklear gui vertex buffer\n");
            SDL_ReleaseGPUBuffer(sdl3_gpu->device, sdl3_gpu->vertex_buffer);
            sdl3_gpu->vertex_buffer = NULL;
        }
        SDL_GPUBufferCreateInfo buf_info = {0};
        buf_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
        buf_info.size = v_size * 2;
        sdl3_gpu->vertex_buffer = SDL_CreateGPUBuffer(sdl3_gpu->device, &buf_info);
        if (sdl3_gpu->vertex_buffer == NULL) {
            SDL_Log("Couldn't create vertex buffer: %s\n", SDL_GetError());
            return;
        }
        sdl3_gpu->vertex_buffer_size = buf_info.size;
    }
    
    if (sdl3_gpu->index_buffer_size < e_size) {
        if (sdl3_gpu->index_buffer) {
            SDL_ReleaseGPUBuffer(sdl3_gpu->device, sdl3_gpu->index_buffer);
            sdl3_gpu->index_buffer = NULL;
        }
        SDL_GPUBufferCreateInfo buf_info = {0};
        buf_info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
        buf_info.size = e_size * 2;
        sdl3_gpu->index_buffer = SDL_CreateGPUBuffer(sdl3_gpu->device, &buf_info);
        if (sdl3_gpu->index_buffer == NULL) {
            SDL_Log("Couldn't create index buffer: %s\n", SDL_GetError());
            return;
        }
        sdl3_gpu->index_buffer_size = buf_info.size;
    }
    
    /* Upload */
    SDL_GPUTransferBufferCreateInfo transfer_info = {0};
    transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer_info.size = v_size + e_size;
    SDL_GPUTransferBuffer *tbuf = SDL_CreateGPUTransferBuffer(sdl3_gpu->device, &transfer_info);
    
    Uint8 *map = SDL_MapGPUTransferBuffer(sdl3_gpu->device, tbuf, false);
    SDL_memcpy(map, vertex_data, v_size);
    SDL_memcpy(map + v_size, element_data, e_size);
    SDL_UnmapGPUTransferBuffer(sdl3_gpu->device, tbuf);
    
    SDL_GPUTransferBufferLocation source = {0};
    source.transfer_buffer = tbuf;
    source.offset = 0;
    SDL_GPUBufferRegion dest = {0};
    dest.buffer = sdl3_gpu->vertex_buffer;
    dest.offset = 0;
    dest.size = v_size;
    SDL_UploadToGPUBuffer(copy_pass, &source, &dest, false);
    source.offset = v_size;
    dest.buffer = sdl3_gpu->index_buffer;
    dest.size = e_size;
    SDL_UploadToGPUBuffer(copy_pass, &source, &dest, false);

    SDL_ReleaseGPUTransferBuffer(sdl3_gpu->device, tbuf);
}

void nk_sdl3_gpu_render_draw(NkSDL3GPU_Device *sdl3_gpu, SDL_GPUCommandBuffer *cmd, SDL_GPURenderPass *pass) {
    struct nk_context *ctx = &sdl3_gpu->ctx;
    
    // 1. Get Actual Pixel Dimensions (Crucial for High-DPI)
    int width, height;
    int pixel_w, pixel_h;
    SDL_GetWindowSize(sdl3_gpu->window, &width, &height);
    SDL_GetWindowSizeInPixels(sdl3_gpu->window, &pixel_w, &pixel_h);

    // Calculate scale factor for High-DPI
    float scale_x = (float)pixel_w / (float)width;
    float scale_y = (float)pixel_h / (float)height;

    /* Bind Pipeline and Shared State */
    SDL_BindGPUGraphicsPipeline(pass, sdl3_gpu->pipeline);
    
    SDL_GPUBufferBinding v_binding = { .buffer = sdl3_gpu->vertex_buffer, .offset = 0 };
    SDL_BindGPUVertexBuffers(pass, 0, &v_binding, 1);
    
    SDL_GPUBufferBinding i_binding = { .buffer = sdl3_gpu->index_buffer, .offset = 0 };
    SDL_BindGPUIndexBuffer(pass, &i_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    /* Viewport matches the full pixel buffer */
    SDL_GPUViewport viewport = {0, 0, (float)pixel_w, (float)pixel_h, 0, 1};
    SDL_SetGPUViewport(pass, &viewport);
    
    /* Projection Matrix: Map 0..width to -1..1 */
    float L = 0.0f, R = (float)width, T = 0.0f, B = (float)height;
    float projection[4][4] = {
        {2.0f/(R-L),    0.0f,          0.0f, 0.0f},
        {0.0f,          2.0f/(T-B),    0.0f, 0.0f},
        {0.0f,          0.0f,          0.5f, 0.0f},
        {(R+L)/(L-R),  (T+B)/(B-T),    0.5f, 1.0f}
    };
    SDL_PushGPUVertexUniformData(cmd, 0, projection, sizeof(projection));
    
    /* Draw commands */
    const struct nk_draw_command *cmd_draw;
    Uint32 offset = 0;
    
    nk_draw_foreach(cmd_draw, ctx, &sdl3_gpu->cmds) {
        if (!cmd_draw->elem_count) continue;
        
        /* Apply High-DPI Scaling to Scissor */
        SDL_Rect scissor;
        scissor.x = (int)(cmd_draw->clip_rect.x * scale_x);
        scissor.y = (int)(cmd_draw->clip_rect.y * scale_y);
        scissor.w = (int)(cmd_draw->clip_rect.w * scale_x);
        scissor.h = (int)(cmd_draw->clip_rect.h * scale_y);

        // Optional: Clamp scissor to window bounds to prevent GPU driver crashes
        scissor.x = SDL_max(0, scissor.x);
        scissor.y = SDL_max(0, scissor.y);
        
        SDL_SetGPUScissor(pass, &scissor);
        
        SDL_GPUTexture *tex = (SDL_GPUTexture*)cmd_draw->texture.ptr;
        if (!tex) tex = sdl3_gpu->font_texture;
        
        SDL_GPUTextureSamplerBinding sampler_binding = {
            .texture = tex,
            .sampler = sdl3_gpu->sampler
        };
        SDL_BindGPUFragmentSamplers(pass, 0, &sampler_binding, 1);
        
        SDL_DrawGPUIndexedPrimitives(pass, cmd_draw->elem_count, 1, offset, 0, 0);
        offset += cmd_draw->elem_count;
    }
    
    nk_clear(ctx);
}

void nk_sdl3_gpu_shutdown(NkSDL3GPU_Device *sdl3_gpu) {
    if (sdl3_gpu == NULL) {
        return;
    }

    nk_free(&sdl3_gpu->ctx);
    nk_buffer_free(&sdl3_gpu->cmds);
    
    if (sdl3_gpu->vertex_buffer) SDL_ReleaseGPUBuffer(sdl3_gpu->device, sdl3_gpu->vertex_buffer);
    if (sdl3_gpu->index_buffer) SDL_ReleaseGPUBuffer(sdl3_gpu->device, sdl3_gpu->index_buffer);
    if (sdl3_gpu->font_texture) SDL_ReleaseGPUTexture(sdl3_gpu->device, sdl3_gpu->font_texture);
    if (sdl3_gpu->sampler) SDL_ReleaseGPUSampler(sdl3_gpu->device, sdl3_gpu->sampler);
    if (sdl3_gpu->pipeline) SDL_ReleaseGPUGraphicsPipeline(sdl3_gpu->device, sdl3_gpu->pipeline);
    if (sdl3_gpu->vertex_shader) SDL_ReleaseGPUShader(sdl3_gpu->device, sdl3_gpu->vertex_shader);
    if (sdl3_gpu->fragment_shader) SDL_ReleaseGPUShader(sdl3_gpu->device, sdl3_gpu->fragment_shader);
    SDL_free(sdl3_gpu);
    sdl3_gpu = NULL;
}

#endif //NK_SDL3_GPU_IMPLEMENTATION
