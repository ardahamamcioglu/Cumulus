![alt text](icon.png)

# Cumulus

My personal learning project for building game engine-like applications.

## Overview

Cumulus is a work-in-progress project created to explore and learn the fundamentals of building game engines and interactive applications. The project integrates several libraries to handle graphics, UI, and scripting.

## Tech Stack

- **SDL3 & SDL_GPU** – Cross-platform window management and GPU-accelerated rendering
- **MicroUI** – Immediate-mode GUI library for building user interfaces, plus a custom
  `microui_ext` widget layer (dropdown, menu bar, color picker, progress bar,
  radio buttons, tooltips)
- **Lua** – Embedded scripting language via C integration
- **cgltf** - glTF loader and writer

## Rendering & portability

The UI backend (`src/microui_sdl3_gpu.h`) picks its shader format at runtime
from the active SDL_GPU backend:

- **Metal** — MSL shaders (embedded source)
- **Vulkan** — SPIR-V shaders (precompiled, `src/microui_spv.h`)

D3D12 (DXIL) is intentionally not yet enabled, since producing DXIL requires
Microsoft's DXC; the GPU device is created with only `SPIRV | MSL` so SDL picks
Metal or Vulkan. To add D3D12 later, wire up DXIL shaders (e.g. via
SDL_shadercross) and re-add `SDL_GPU_SHADERFORMAT_DXIL` to the device format
flags in `src/app.c`.

The SPIR-V blob is generated from the editable GLSL sources in
[`shaders/`](shaders/) — regenerate it with:

```bash
python3 shaders/compile_spirv.py   # requires: brew install glslang
```

## Building

```bash
./build.sh
```

## Status

🚧 **Work in Progress** – This project is in active development and may evolve in unexpected directions. It might eventually be used as a foundation for developing other applications.

## License

This is a personal learning project.
