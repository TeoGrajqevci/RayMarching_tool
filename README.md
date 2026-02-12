<img width="1252" height="1287" alt="Capture d’écran 2026-02-12 à 16 06 47" src="https://github.com/user-attachments/assets/0de5a76f-2e7f-489e-9d3a-5e1fcadc01b3" /># Ray Marching Tool

A **3D modeling tool** written in **C++** and using **OpenGL**, leveraging **Signed Distance Fields (SDFs)** to create, blend, and manipulate primitive shapes.

---

## Table of Contents

1. [Features](#features)
2. [Preview](#preview)
3. [Getting Started](#getting-started)
4. [Building from Source](#building-from-source)
5. [Usage](#usage)

---

## Features

- **SDF Modeling**
  - Add or remove primitive shapes (currently sphere, box, cylinder, torus, with more on the way).
  - Apply blending operations: **Union**, **Subtract**, **Intersect**.
  - Control blending smoothness per shape.

- **Shape Hierarchy**
  - Blend modes depend on their position in the shape directory hierarchy.

- **Color & Lighting Controls**
  - Change the color of shapes individually.
  - Adjust the light color, position, and intensity.

- **Interactive Interface**
  - Real-time feedback using an ImGui-based GUI.
  - Transform shapes (translate, rotate, scale) via a user-friendly interface and key-bingings.

- **Rendering**
  - OpenGL raster-based rendering (improvements in progress).
  - **HDRi support** (in progress).
  - **Path tracing renderer** (in progress).

---

## Preview

![Uploading raymarch_cornel_box.png…]()

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
cmake --build . --parallel
```

or

```bash
cmake --build . --parallel <N>
```

After compilation, you will get an executable named `RayMarchingTool`.

### Execution

Run the compiled binary from the build directory:

```bash
./RayMarchingTool
```

---
