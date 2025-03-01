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

Ces instructions vont vous aider à mettre en place une copie du projet sur votre machine locale pour le développement et les tests.

### Prérequis
- Compilateur C++ supportant C++11 ou supérieur (ex. gcc, clang ou MSVC).
- CMake 3.11 ou supérieur.
- Git pour cloner le dépôt (optionnel, mais recommandé).

### Clonage du dépôt
git clone https://github.com/TeoGrajqevci/RayMarching_tool.git
cd RayMarching_tool

### Créer un répertoire de build
À partir de la racine du projet :
```bash
mkdir build
cd build
```

### Configuration avec CMake
Lancez CMake pour configurer le projet et télécharger les dépendances externes (GLAD, ImGui, GLFW, stb) :
```bash
cmake ..
```

### Compilation
Une fois la configuration terminée :
```bash
make
```

Après la compilation, vous obtiendrez un exécutable nommé RayMarchingTool (ou le nom défini dans le fichier CMake).

### Exécution
Exécutez le binaire compilé depuis le répertoire build :
```bash
./RayMarchingTool
```

## Roadmap

- **HDRi Support**  
  Implémenter correctement les environnement maps HDR pour un réalisme lumineux amélioré.
- **Path Tracing Renderer**  
  Développer un mode de path tracing optionnel pour un rendu offline de haute qualité.
- **Additional Blend Modes**  
  Expérimenter différentes opérations de fusion SDF au-delà de l'union, la soustraction et l'intersection.
- **Multiple Lights**  
  Permettre plusieurs sources de lumière avec des paramètres ajustables (couleur, intensité, ombres, etc.).
- **More Primitive Shapes**  
  Étendre la bibliothèque de primitives SDF (cubes, cylindres, cônes, torus, etc.).
- **Advanced Material System**  
  Activer des propriétés de surface avancées telles que le métallique, la rugosité et la cartographie de textures.

---


