#define CGLTF_IMPLEMENTATION
#include <SDL3/SDL.h>
#include <cgltf.h>

struct cgltf_data *model_load(const char *path)
{
    cgltf_options options = {0};
    cgltf_data *data = NULL;

    cgltf_result result = cgltf_parse_file(&options, path, &data);
    if (result != cgltf_result_success)
    {
        SDL_Log("Failed to parse glTF '%s' (error %d)", path, (int)result);
        return NULL;
    }

    result = cgltf_load_buffers(&options, data, path);
    if (result != cgltf_result_success)
    {
        SDL_Log("Failed to load glTF buffers '%s' (error %d)", path, (int)result);
        cgltf_free(data);
        return NULL;
    }

    /* Log summary */
    SDL_Log("--- Model: %s ---", path);
    SDL_Log("  Nodes:     %zu", data->nodes_count);
    SDL_Log("  Meshes:    %zu", data->meshes_count);
    SDL_Log("  Materials: %zu", data->materials_count);
    for (size_t i = 0; i < data->meshes_count; i++)
    {
        const cgltf_mesh *mesh = &data->meshes[i];
        SDL_Log("  Mesh[%zu]: %s", i, mesh->name ? mesh->name : "(unnamed)");
        for (size_t j = 0; j < mesh->primitives_count; j++)
        {
            const cgltf_primitive *prim = &mesh->primitives[j];
            size_t verts = prim->attributes_count > 0 && prim->attributes[0].data ? prim->attributes[0].data->count : 0;
            size_t idxs = prim->indices ? prim->indices->count : 0;
            SDL_Log("    Prim[%zu]: %s  verts=%zu  indices=%zu", j,
                    prim->type == cgltf_primitive_type_triangles ? "triangles" : "other", verts, idxs);
        }
    }

    return data;
}

void model_free(struct cgltf_data *model)
{
    if (model)
    {
        cgltf_free(model);
    }
}
