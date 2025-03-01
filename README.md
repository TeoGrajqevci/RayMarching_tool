Ray Marching Tool

A 3D modeling application written in C++ and OpenGL, leveraging Signed Distance Fields (SDFs) to create, blend, and manipulate primitive shapes. This project provides an interactive environment where shapes can be added or removed, and blended using union, subtraction, and intersection—each with a customizable smooth factor. You can also change colors and lighting parameters on the fly.

Note: This is an early-stage project. HDRi support, a more advanced raster-based renderer, and a path tracer are in progress. More blend modes, multiple lights, additional parameters, and extra shapes are also planned.
Table of Contents

Features
Preview
Getting Started
Building from Source
Usage
Roadmap
Contributing
License
Features

SDF Modeling
Add or remove primitive shapes (currently spheres, with more on the way).
Apply blending operations: union, subtract, intersect.
Control blending smoothness per shape.
Shape Hierarchy
Blend modes are dependent on their position in the shape directory hierarchy.
Color & Lighting Controls
Change the color of shapes individually.
Adjust the light color, position, and intensity.
Interactive Interface
Real-time feedback using ImGui-based GUI.
Transform shapes (translate, rotate, scale) via a user-friendly interface.
Rendering
OpenGL raster-based rendering (in development for improvement).
HDRi support (in progress).
Path tracing renderer (in progress).
Preview

Below is a screenshot demonstrating the current interface. You can see a sphere in the scene, the GUI panels for lighting and shape parameters, and a shape directory for managing your objects:

(Replace the above link with your actual screenshot or remove if hosting images directly in the repository.)

Getting Started

These instructions will help you get a copy of the project up and running on your local machine for development and testing purposes.

Prerequisites
C++ Compiler supporting C++11 or later (e.g., gcc, clang, or MSVC).
CMake 3.11 or higher.
Git for cloning the repository (optional, but recommended).
Cloning the Repository
git clone https://github.com/<YourUsername>/<YourRepoName>.git
cd <YourRepoName>
Building from Source

Create a Build Directory
From the root of the project:
mkdir build
cd build
Configure with CMake
Run CMake to configure the project and download external dependencies (GLAD, ImGui, GLFW, stb):
cmake ..
Compile
Once the configuration is complete:
make
After the build finishes, you will have an executable named RefactoredProject (or the name you set in the CMake file).
Run
Execute the compiled binary from the build directory:
./RefactoredProject
Usage

When you launch RefactoredProject, you will see:

A 3D viewport with a grid and axes.
A Shape Directory panel on the right where you can add, remove, and arrange shapes.
A Transform panel (or combined with the main viewport) where you can modify position, rotation, and scale.
A Lighting panel to control light color and position.
A Material or Color panel to change shape appearance.
Basic Controls
Add Shape: Click the + button (or similar control) in the Shape Directory panel. Select a primitive (e.g., Sphere).
Blend Modes: Choose from Union, Subtract, or Intersect. Adjust the smooth factor to soften edges between shapes.
Transform:
Select a shape in the directory.
Use translation, rotation, and scale tools (often bound to keys like W, E, R or accessible via the GUI).
Lighting:
Adjust the light’s color and position.
Future support for multiple lights is planned.
(For more detailed instructions, please refer to the in-app tooltips or any additional documentation you may add.)

Roadmap

HDRi Support
Properly implement HDR environment maps for improved lighting realism.
Path Tracing Renderer
Develop an optional path tracing mode for high-quality offline rendering.
Additional Blend Modes
Experiment with different SDF blending operations beyond union, subtract, and intersect.
Multiple Lights
Allow multiple light sources with adjustable parameters (color, intensity, shadows, etc.).
More Primitive Shapes
Extend the library of SDF primitives (cubes, cylinders, cones, torus, etc.).
Advanced Material System
Enable advanced surface properties like metallic, roughness, and texture mapping.
