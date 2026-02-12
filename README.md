<img width="1250" height="1261" alt="raymarch_cornel_box" src="https://github.com/user-attachments/assets/b16b9ccd-1d69-4eb1-b09d-3613eb798664" /># Ray Marching Tool

Ray Marching Tool is an interactive SDF modeling app written in C++ with OpenGL + ImGui.
It supports real-time ray marching, a path tracer mode, shape modifiers, and OBJ export via marching cubes.

## Preview

<img width="1250" height="1261" alt="raymarch_cornel_box" src="https://github.com/user-attachments/assets/3b9e211a-e349-4796-b135-ab4ab6235bd1" />


## Current Features

- SDF primitives: Sphere, Box, Torus, Cylinder, Cone, Mandelbulb
- Blend modes: Smooth Union, Smooth Subtraction, Smooth Intersection
- Modifiers: Bend, Twist, Mirror (with axis and smoothness controls)
- Transform workflows:
  - Keyboard transform mode (`G` / `R` / `S`)
  - Viewport gizmo (World/Local mode)
  - Axis constraints (`X` / `Y` / `Z`)
- Rendering modes:
  - Real-time ray marching renderer
  - Path tracer renderer with accumulation
  - Optional OpenImageDenoise denoising (when a GPU-backed OIDN runtime is detected)
- Material controls:
  - Albedo, Metallic, Roughness, Emission, Transmission, IOR
- Scene operations:
  - Multi-select, duplicate, reorder, rename
  - Undo/redo (`Ctrl+Z`, `Ctrl+Y`, `Ctrl+Shift+Z`)
- Mesh export:
  - Export current SDF scene to `.obj` using marching cubes

## Requirements

- CMake `>= 3.11`
- C++11 compiler (clang/gcc/MSVC)
- OpenGL 3.3 Core compatible GPU/driver
- Internet access during first CMake configure (dependencies are fetched automatically)

Optional:

- OpenMP for faster CPU-side mesh export (`-DRMT_USE_OPENMP=ON`)
- OpenImageDenoise with a GPU backend for path-tracer denoising

## Build

```bash
mkdir -p build
cd build
cmake ..
cmake --build . --parallel
```

Run from the build directory:

```bash
./RayMarchingTool
```

## Useful CMake Options

- `-DRMT_FAST_FIRST_BUILD=ON|OFF`
  - `ON` (default): shallow clones + skip dependency update checks
- `-DRMT_USE_OPENMP=ON|OFF`
  - `ON`: enables OpenMP for heavy CPU loops (notably mesh export), when compiler support exists

Example:

```bash
cmake .. -DRMT_USE_OPENMP=ON -DRMT_FAST_FIRST_BUILD=ON
```

## Command-Line Benchmark Option

You can auto-generate a benchmark scene with an even mix of shape types:

```bash
./RayMarchingTool --benchmark-even-mix=1000
```

or

```bash
./RayMarchingTool --benchmark-even-mix 1000
```

## Controls

- `LMB drag`: orbit camera
- `Shift + LMB drag`: pan camera
- `Mouse wheel`: zoom
- `Shift + A`: open Add Shape popup
- `LMB`: select shape (viewport picking)
- `Shift + Click`: toggle multi-selection
- `G` / `R` / `S`: enter move / rotate / scale modes
- `X` / `Y` / `Z` during transform: axis constraint
- `Esc`: cancel active transform
- `Shift + D`: duplicate selected shapes
- `X` (outside transform mode): delete selection
- `C`: focus camera on last selected shape
- `H`: toggle UI visibility
- `Ctrl + Z`: undo
- `Ctrl + Y` or `Ctrl + Shift + Z`: redo

## Exporting Meshes

Use the top toolbar button `Export OBJ`:

1. Set output filename/path
2. Choose export resolution
3. Set bounding box size
4. Export to OBJ

Higher resolutions produce more detail but require more time and memory.
