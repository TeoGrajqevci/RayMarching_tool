

# Ray Marching Tool

A **3D modeling application** written in **C++** and **OpenGL**, leveraging **Signed Distance Fields (SDFs)** to create, blend, and manipulate primitive shapes. This project provides an interactive environment where shapes can be added or removed, and blended using union, subtraction, and intersection—each with a customizable smooth factor. You can also change colors and lighting parameters on the fly.

> **Note**: This is an early-stage project. Proper HDRi support, an improved raster-based renderer, and a path tracing renderer are in progress. Future plans include additional blend modes, multiple lights, more parameters, and a wider range of primitive shapes.

---

## Table of Contents

1. [Features](#features)  
2. [Preview](#preview)  
3. [Getting Started](#getting-started)  
4. [Building from Source](#building-from-source)  
5. [Usage](#usage)  
6. [Roadmap](#roadmap)  

---

## Features

- **SDF Modeling**  
  - Add or remove primitive shapes (currently spheres, with more on the way).
  - Apply blending operations: **Union**, **Subtract**, **Intersect**.
  - Control blending smoothness per shape.

- **Shape Hierarchy**  
  - Blend modes depend on their position in the shape directory hierarchy.

- **Color & Lighting Controls**  
  - Change the color of shapes individually.
  - Adjust the light color, position, and intensity.

- **Interactive Interface**  
  - Real-time feedback using an ImGui-based GUI.
  - Transform shapes (translate, rotate, scale) via a user-friendly interface.

- **Rendering**  
  - OpenGL raster-based rendering (improvements in progress).
  - **HDRi support** (in progress).
  - **Path tracing renderer** (in progress).

---

## Getting Started

These instructions will help you set up a copy of the project on your local machine for development and testing.

### Prerequisites
- A C++ compiler supporting C++11 or later (e.g., gcc, clang, or MSVC).
- CMake 3.11 or higher.
- Git for cloning the repository (optional but recommended).

### Cloning the Repository
```bash
git clone https://github.com/TeoGrajqevci/RayMarching_tool.git
cd RayMarching_tool
```

### Creating a Build Directory
From the root of the project:
```bash
mkdir build
cd build
```

### Configuring with CMake
Run CMake to configure the project and download external dependencies (GLAD, ImGui, GLFW, stb):
```bash
cmake ..
```

### Compilation
Once the configuration is complete:
```bash
make
```

After compilation, you will get an executable named `RayMarchingTool` (or the name defined in the CMake file).

### Execution
Run the compiled binary from the build directory:
```bash
./RayMarchingTool
```

---

## Roadmap

- **HDRi Support**  
  Implement proper HDR environment maps for improved lighting realism.
- **Path Tracing Renderer**  
  Develop an optional path tracing mode for high-quality offline rendering.
- **Additional Blend Modes**  
  Experiment with different SDF blend operations beyond union, subtraction, and intersection.
- **Multiple Lights**  
  Allow multiple light sources with adjustable parameters (color, intensity, shadows, etc.).
- **More Primitive Shapes**  
  Expand the SDF primitive library (cubes, cylinders, cones, torus, etc.).
- **Advanced Material System**  
  Enable advanced surface properties such as metallic, roughness, and texture mapping.

---
