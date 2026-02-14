cmake_minimum_required(VERSION 3.11)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

# Set policy to handle compatibility with older CMake versions in dependencies
if(POLICY CMP0048)
    cmake_policy(SET CMP0048 NEW)
endif()

project(RayMarchingTool)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the build type." FORCE)
    message(STATUS "No build type selected, defaulting to RelWithDebInfo.")
endif()

include(FetchContent)

# Reduce first-time setup cost by cloning only the latest commit for deps.
option(RMT_FAST_FIRST_BUILD "Use shallow dependency clones and skip update checks" ON)
option(RMT_USE_OPENMP "Enable OpenMP parallel loops for heavy CPU tasks (mesh export)." OFF)
if(RMT_FAST_FIRST_BUILD)
    set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "Skip dependency update checks" FORCE)
endif()

# -------------------------------
# Fetch glad
# -------------------------------
FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG v0.1.36  # Use a stable v0.x version compatible with your project structure
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glad)

# -------------------------------
# Fetch imgui
# -------------------------------
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG master
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(imgui)

# -------------------------------
# Fetch ImGuizmo
# -------------------------------
FetchContent_Declare(
    imguizmo
    GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo.git
    GIT_TAG master
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(imguizmo)

# -------------------------------
# Fetch glfw (will be built as part of the project)
# -------------------------------
# Avoid building GLFW extras that are not needed for this application.
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG master
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glfw)

# -------------------------------
# Fetch stb (for stb_image.h)
# -------------------------------
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master  # Or specify a commit/tag if needed
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(stb)

# -------------------------------
# Fetch assimp (OBJ/FBX import)
# -------------------------------
find_package(assimp QUIET)
if((NOT assimp_FOUND AND NOT ASSIMP_FOUND) AND NOT TARGET assimp::assimp AND NOT TARGET assimp)
    set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_FBX_IMPORTER ON CACHE BOOL "" FORCE)

    FetchContent_Declare(
        assimp
        GIT_REPOSITORY https://github.com/assimp/assimp.git
        GIT_TAG v5.4.3
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(assimp)
endif()

# -------------------------------
# Source manifests (modular layout)
# -------------------------------
include(${PROJECT_SOURCE_DIR}/cmake/sources_app.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/sources_common.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/sources_scene.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/sources_rendering.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/sources_input.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/sources_ui.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/sources_io.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/sources_platform.cmake)

# -------------------------------
# Create the executable target
# -------------------------------
add_executable(${PROJECT_NAME}
    ${RMT_APP_SOURCES}
    ${RMT_COMMON_SOURCES}
    ${RMT_SCENE_SOURCES}
    ${RMT_RENDERING_SOURCES}
    ${RMT_INPUT_SOURCES}
    ${RMT_UI_SOURCES}
    ${RMT_IO_SOURCES}
    ${RMT_PLATFORM_SOURCES}
    third_party/mesh2sdf/makelevelset3.cpp
)

# -------------------------------
# Include directories
# -------------------------------
# Your project's headers
target_include_directories(${PROJECT_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/third_party/mesh2sdf
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
    ${imguizmo_SOURCE_DIR}/ImGuizmo.cpp
)
# Include imgui directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${imguizmo_SOURCE_DIR}
)

# -------------------------------
# Link GLFW and OpenGL
# -------------------------------
find_package(OpenGL REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw OpenGL::GL)

if(TARGET assimp::assimp)
    target_link_libraries(${PROJECT_NAME} PRIVATE assimp::assimp)
elseif(TARGET assimp)
    target_link_libraries(${PROJECT_NAME} PRIVATE assimp)
elseif(assimp_FOUND OR ASSIMP_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE assimp)
else()
    message(FATAL_ERROR "Assimp target was not found. OBJ/FBX mesh import requires assimp.")
endif()

if(RMT_USE_OPENMP)
    find_package(OpenMP QUIET)
    if(OpenMP_CXX_FOUND)
        target_link_libraries(${PROJECT_NAME} PRIVATE OpenMP::OpenMP_CXX)
        target_compile_definitions(${PROJECT_NAME} PRIVATE RMT_USE_OPENMP=1)
        message(STATUS "OpenMP enabled for CPU-side heavy loops.")
    else()
        message(WARNING "RMT_USE_OPENMP is ON but OpenMP was not found; continuing without OpenMP.")
    endif()
endif()

# -------------------------------
# Optional OpenImageDenoise integration (for path tracer denoising)
# -------------------------------
find_package(OpenImageDenoise CONFIG QUIET)
if(NOT OpenImageDenoise_FOUND AND EXISTS "/opt/homebrew/opt/open-image-denoise/lib/cmake")
    file(GLOB OIDN_CONFIG_DIR "/opt/homebrew/opt/open-image-denoise/lib/cmake/OpenImageDenoise-*")
    if(OIDN_CONFIG_DIR)
        find_package(OpenImageDenoise CONFIG QUIET PATHS ${OIDN_CONFIG_DIR})
    endif()
endif()

if(OpenImageDenoise_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenImageDenoise)
    target_compile_definitions(${PROJECT_NAME} PRIVATE RMT_HAS_OIDN=1)
    message(STATUS "OpenImageDenoise found: path tracer denoising enabled.")

    set(RMT_OIDN_HAS_GPU_BACKEND OFF)
    if((DEFINED OIDN_DEVICE_SYCL AND OIDN_DEVICE_SYCL) OR
       (DEFINED OIDN_DEVICE_CUDA AND OIDN_DEVICE_CUDA) OR
       (DEFINED OIDN_DEVICE_HIP AND OIDN_DEVICE_HIP) OR
       (DEFINED OIDN_DEVICE_METAL AND OIDN_DEVICE_METAL))
        set(RMT_OIDN_HAS_GPU_BACKEND ON)
    endif()

    # Some OIDN packages (notably Metal-only builds) may not export OIDN_DEVICE_* variables.
    if(NOT RMT_OIDN_HAS_GPU_BACKEND)
        if(DEFINED OpenImageDenoise_DIR)
            get_filename_component(_rmt_oidn_prefix "${OpenImageDenoise_DIR}/../../.." ABSOLUTE)
            file(GLOB _rmt_oidn_gpu_plugins
                "${_rmt_oidn_prefix}/lib/libOpenImageDenoise_device_*.dylib"
                "${_rmt_oidn_prefix}/lib/libOpenImageDenoise_device_*.so"
                "${_rmt_oidn_prefix}/lib/OpenImageDenoise_device_*.dll")
            if(_rmt_oidn_gpu_plugins)
                set(RMT_OIDN_HAS_GPU_BACKEND ON)
            endif()
        endif()
    endif()

    if(RMT_OIDN_HAS_GPU_BACKEND)
        message(STATUS "OpenImageDenoise GPU backend support detected.")
    else()
        message(WARNING "OpenImageDenoise was built CPU-only. GPU denoising device selection will not be available until OIDN is built with a GPU backend.")
    endif()
else()
    message(WARNING "OpenImageDenoise not found: path tracer will run without OIDN denoising.")
endif()

# -------------------------------
# Copy shaders directory to the build directory
# -------------------------------
file(COPY ${PROJECT_SOURCE_DIR}/shaders DESTINATION ${CMAKE_BINARY_DIR})
