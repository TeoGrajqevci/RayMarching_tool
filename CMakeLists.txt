cmake_minimum_required(VERSION 3.11)
project(RayMarchingTool)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# -------------------------------
# Fetch glad
# -------------------------------
FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git

)
FetchContent_MakeAvailable(glad)

# -------------------------------
# Fetch imgui
# -------------------------------
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git

)
FetchContent_MakeAvailable(imgui)

# -------------------------------
# Fetch glfw (will be built as part of the project)
# -------------------------------
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git

)
FetchContent_MakeAvailable(glfw)

# -------------------------------
# Fetch stb (for stb_image.h)
# -------------------------------
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master  # Or specify a commit/tag if needed
)
FetchContent_MakeAvailable(stb)

# -------------------------------
# Create the executable target
# -------------------------------
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/Shader.cpp
    src/Texture.cpp
    src/ImGuiLayer.cpp
    src/Shapes.cpp
    src/Input.cpp
    src/Utilities.cpp
    src/Renderer.cpp
    src/GuiManager.cpp
)

# -------------------------------
# Include directories
# -------------------------------
# Your project's headers
target_include_directories(${PROJECT_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    /opt/homebrew/include
)
# stb: for stb_image.h
target_include_directories(${PROJECT_NAME} PRIVATE ${stb_SOURCE_DIR})
# glad: for the glad headers
target_include_directories(${PROJECT_NAME} PRIVATE ${glad_SOURCE_DIR}/include)

# -------------------------------
# Link glad library
# -------------------------------
target_link_libraries(${PROJECT_NAME} PRIVATE glad)

# -------------------------------
# Integrate imgui
# -------------------------------
# Add imgui source files directly to your target
target_sources(${PROJECT_NAME} PRIVATE
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
# Include imgui directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

# -------------------------------
# Link GLFW and OpenGL
# -------------------------------
find_package(OpenGL REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw OpenGL::GL)

# -------------------------------
# Copy shaders directory to the build directory
# -------------------------------
file(COPY ${PROJECT_SOURCE_DIR}/shaders DESTINATION ${CMAKE_BINARY_DIR})
