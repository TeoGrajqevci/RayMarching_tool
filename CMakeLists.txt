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
# Fetch mesh2sdf (mesh -> SDF conversion backend)
# -------------------------------
option(RMT_AUTO_BOOTSTRAP_MESH2SDF "Automatically clone mesh2sdf into third_party/ on first configure if missing" ON)
set(RMT_MESH2SDF_DIR "${PROJECT_SOURCE_DIR}/third_party/mesh2sdf")
set(RMT_MESH2SDF_INCLUDE_DIR "")
set(RMT_MESH2SDF_CPP "")

if(EXISTS "${RMT_MESH2SDF_DIR}/makelevelset3.cpp")
    set(RMT_MESH2SDF_INCLUDE_DIR "${RMT_MESH2SDF_DIR}")
    set(RMT_MESH2SDF_CPP "${RMT_MESH2SDF_DIR}/makelevelset3.cpp")
elseif(EXISTS "${RMT_MESH2SDF_DIR}/csrc/makelevelset3.cpp")
    set(RMT_MESH2SDF_INCLUDE_DIR "${RMT_MESH2SDF_DIR}/csrc")
    set(RMT_MESH2SDF_CPP "${RMT_MESH2SDF_DIR}/csrc/makelevelset3.cpp")
endif()

if(NOT RMT_MESH2SDF_CPP)
    if(RMT_AUTO_BOOTSTRAP_MESH2SDF)
        message(STATUS "mesh2sdf source not found. Cloning into ${RMT_MESH2SDF_DIR}...")
        FetchContent_Declare(
            rmt_mesh2sdf
            GIT_REPOSITORY https://github.com/wang-ps/mesh2sdf.git
            GIT_TAG 33f71778a81a369a7a3fee5a3a4dabcbb6287e57
            GIT_SHALLOW TRUE
            SOURCE_DIR "${RMT_MESH2SDF_DIR}"
        )
        FetchContent_MakeAvailable(rmt_mesh2sdf)

        if(EXISTS "${RMT_MESH2SDF_DIR}/makelevelset3.cpp")
            set(RMT_MESH2SDF_INCLUDE_DIR "${RMT_MESH2SDF_DIR}")
            set(RMT_MESH2SDF_CPP "${RMT_MESH2SDF_DIR}/makelevelset3.cpp")
        elseif(EXISTS "${RMT_MESH2SDF_DIR}/csrc/makelevelset3.cpp")
            set(RMT_MESH2SDF_INCLUDE_DIR "${RMT_MESH2SDF_DIR}/csrc")
            set(RMT_MESH2SDF_CPP "${RMT_MESH2SDF_DIR}/csrc/makelevelset3.cpp")
        endif()
    else()
        message(FATAL_ERROR "mesh2sdf source is missing. Re-run with -DRMT_AUTO_BOOTSTRAP_MESH2SDF=ON or provide the third_party/mesh2sdf repo.")
    endif()
endif()

if(NOT RMT_MESH2SDF_CPP)
    message(FATAL_ERROR "mesh2sdf bootstrap failed: expected makelevelset3.cpp at either third_party/mesh2sdf/ or third_party/mesh2sdf/csrc/.")
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
    ${RMT_MESH2SDF_CPP}
)

# -------------------------------
# Include directories
# -------------------------------
# Your project's headers
target_include_directories(${PROJECT_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${RMT_MESH2SDF_INCLUDE_DIR}
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
option(RMT_AUTO_BOOTSTRAP_OIDN_GPU "Automatically build a local GPU-capable OIDN if no local install is found" ON)

set(RMT_OIDN_FROM_SOURCE OFF)
set(RMT_OIDN_LOCAL_ROOT "${PROJECT_SOURCE_DIR}/third_party/oidn-local")
set(RMT_OIDN_LOCAL_SOURCE_DIR "${RMT_OIDN_LOCAL_ROOT}/oidn-src")
set(RMT_OIDN_LOCAL_INSTALL_DIR "${RMT_OIDN_LOCAL_ROOT}/oidn-install")
set(RMT_LOCAL_OIDN_CONFIG_DIR "")
file(GLOB RMT_LOCAL_OIDN_CONFIG_CANDIDATES
    "${RMT_OIDN_LOCAL_INSTALL_DIR}/lib/cmake/OpenImageDenoise-*/OpenImageDenoiseConfig.cmake")
if(RMT_LOCAL_OIDN_CONFIG_CANDIDATES)
    list(GET RMT_LOCAL_OIDN_CONFIG_CANDIDATES 0 RMT_LOCAL_OIDN_CONFIG_FILE)
    get_filename_component(RMT_LOCAL_OIDN_CONFIG_DIR "${RMT_LOCAL_OIDN_CONFIG_FILE}" DIRECTORY)
endif()

if(RMT_LOCAL_OIDN_CONFIG_DIR)
    find_package(OpenImageDenoise CONFIG QUIET
        PATHS "${RMT_LOCAL_OIDN_CONFIG_DIR}"
        NO_DEFAULT_PATH)
endif()

if(NOT OpenImageDenoise_FOUND AND APPLE AND RMT_AUTO_BOOTSTRAP_OIDN_GPU)
    message(STATUS "OpenImageDenoise local GPU build not found. Bootstrapping OIDN (Metal) from source in ${RMT_OIDN_LOCAL_SOURCE_DIR}...")

    set(OIDN_DEVICE_CPU ON CACHE BOOL "" FORCE)
    set(OIDN_DEVICE_METAL ON CACHE BOOL "" FORCE)
    set(OIDN_DEVICE_SYCL OFF CACHE BOOL "" FORCE)
    set(OIDN_DEVICE_CUDA OFF CACHE BOOL "" FORCE)
    set(OIDN_DEVICE_HIP OFF CACHE BOOL "" FORCE)
    set(OIDN_APPS OFF CACHE BOOL "" FORCE)
    set(OIDN_FILTER_RT ON CACHE BOOL "" FORCE)
    set(OIDN_FILTER_RTLIGHTMAP OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        rmt_oidn
        GIT_REPOSITORY https://github.com/RenderKit/oidn.git
        GIT_TAG v2.4.1
        GIT_SHALLOW TRUE
        GIT_SUBMODULES weights
        GIT_SUBMODULES_RECURSE FALSE
        SOURCE_DIR "${RMT_OIDN_LOCAL_SOURCE_DIR}"
    )
    FetchContent_MakeAvailable(rmt_oidn)

    if(TARGET OpenImageDenoise)
        set(OpenImageDenoise_FOUND TRUE)
        set(RMT_OIDN_FROM_SOURCE ON)
        message(STATUS "OpenImageDenoise bootstrapped from source with Metal backend.")
    endif()
endif()

if(NOT OpenImageDenoise_FOUND)
    find_package(OpenImageDenoise CONFIG QUIET)
endif()
if(NOT OpenImageDenoise_FOUND AND EXISTS "/opt/homebrew/opt/open-image-denoise/lib/cmake")
    file(GLOB OIDN_CONFIG_DIR "/opt/homebrew/opt/open-image-denoise/lib/cmake/OpenImageDenoise-*")
    if(OIDN_CONFIG_DIR)
        find_package(OpenImageDenoise CONFIG QUIET PATHS ${OIDN_CONFIG_DIR})
    endif()
endif()

if(OpenImageDenoise_FOUND OR TARGET OpenImageDenoise)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenImageDenoise)
    target_compile_definitions(${PROJECT_NAME} PRIVATE RMT_HAS_OIDN=1)
    message(STATUS "OpenImageDenoise found: path tracer denoising enabled.")

    set(RMT_OIDN_HAS_GPU_BACKEND OFF)
    if(RMT_OIDN_FROM_SOURCE)
        if(TARGET OpenImageDenoise_device_metal OR TARGET OpenImageDenoise_device_cuda OR TARGET OpenImageDenoise_device_hip OR TARGET OpenImageDenoise_device_sycl)
            set(RMT_OIDN_HAS_GPU_BACKEND ON)
        endif()
    endif()

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
                "${_rmt_oidn_prefix}/lib/libOpenImageDenoise_device_metal*.dylib"
                "${_rmt_oidn_prefix}/lib/libOpenImageDenoise_device_cuda*.dylib"
                "${_rmt_oidn_prefix}/lib/libOpenImageDenoise_device_hip*.dylib"
                "${_rmt_oidn_prefix}/lib/libOpenImageDenoise_device_sycl*.dylib"
                "${_rmt_oidn_prefix}/lib/libOpenImageDenoise_device_cuda*.so*"
                "${_rmt_oidn_prefix}/lib/libOpenImageDenoise_device_hip*.so*"
                "${_rmt_oidn_prefix}/lib/libOpenImageDenoise_device_sycl*.so*"
                "${_rmt_oidn_prefix}/bin/OpenImageDenoise_device_cuda*.dll"
                "${_rmt_oidn_prefix}/bin/OpenImageDenoise_device_hip*.dll"
                "${_rmt_oidn_prefix}/bin/OpenImageDenoise_device_sycl*.dll")
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

    if(RMT_OIDN_FROM_SOURCE)
        if(TARGET OpenImageDenoise)
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "$<TARGET_FILE:OpenImageDenoise>"
                        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
                VERBATIM)
        endif()
        if(TARGET OpenImageDenoise_core)
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "$<TARGET_FILE:OpenImageDenoise_core>"
                        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
                VERBATIM)
        endif()
        if(TARGET OpenImageDenoise_device_cpu)
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "$<TARGET_FILE:OpenImageDenoise_device_cpu>"
                        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
                VERBATIM)
        endif()
        if(TARGET OpenImageDenoise_device_metal)
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "$<TARGET_FILE:OpenImageDenoise_device_metal>"
                        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
                VERBATIM)
        endif()
        message(STATUS "OpenImageDenoise runtime libraries from source build will be copied next to the executable.")
    elseif(DEFINED OpenImageDenoise_DIR)
        get_filename_component(_rmt_oidn_prefix "${OpenImageDenoise_DIR}/../../.." ABSOLUTE)
        set(_rmt_oidn_runtime_libs)
        if(APPLE)
            file(GLOB _rmt_oidn_runtime_libs "${_rmt_oidn_prefix}/lib/libOpenImageDenoise*.dylib")
        elseif(UNIX)
            file(GLOB _rmt_oidn_runtime_libs "${_rmt_oidn_prefix}/lib/libOpenImageDenoise*.so*")
        elseif(WIN32)
            file(GLOB _rmt_oidn_runtime_libs "${_rmt_oidn_prefix}/bin/OpenImageDenoise*.dll")
        endif()

        if(_rmt_oidn_runtime_libs)
            foreach(_rmt_oidn_lib IN LISTS _rmt_oidn_runtime_libs)
                add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                            "${_rmt_oidn_lib}"
                            "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
                    VERBATIM)
            endforeach()
            message(STATUS "OpenImageDenoise runtime libraries will be copied next to the executable.")
        endif()
    endif()
else()
    message(WARNING "OpenImageDenoise not found: path tracer will run without OIDN denoising.")
endif()

# -------------------------------
# Copy shaders directory to the build directory
# -------------------------------
file(COPY ${PROJECT_SOURCE_DIR}/shaders DESTINATION ${CMAKE_BINARY_DIR})

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${PROJECT_SOURCE_DIR}/shaders
            ${CMAKE_BINARY_DIR}/shaders
    COMMENT "Syncing shaders to build directory"
)
