
# RefactoredProject

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
7. [Contributing](#contributing)  
8. [License](#license)  
9. [CMakeLists.txt Reference](#cmakeliststxt-reference)

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

## Preview

Below is a screenshot demonstrating the current interface. You can see a **sphere** in the scene, the **GUI** panels for lighting and shape parameters, and a **shape directory** for managing your objects:

![RefactoredProject Screenshot](https://via.placeholder.com/800x600?text=RefactoredProject+Preview)

*(Replace the above link with your actual screenshot or remove if hosting images directly in the repository.)*

---

## Getting Started

These instructions will help you get a copy of the project up and running on your local machine for development and testing purposes.

### Prerequisites

- **C++ Compiler** supporting C++11 or later (e.g., `gcc`, `clang`, or MSVC).
- **CMake** 3.11 or higher.
- **Git** for cloning the repository (optional, but recommended).



