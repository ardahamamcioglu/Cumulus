#ifndef CUMULUS_MODEL_IMPORT_H
#define CUMULUS_MODEL_IMPORT_H

/* Opaque cgltf data handle */
struct cgltf_data;

/* Load glTF file via cgltf. Returns handle or NULL. Logs model summary. */
struct cgltf_data *model_load(const char *path);

/* Free loaded model. Safe to call with NULL. */
void model_free(struct cgltf_data *model);

#endif /* CUMULUS_MODEL_IMPORT_H */
