# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/Users/teograjqevci/dev/RayMarching_tool/build/_deps/glad-src")
  file(MAKE_DIRECTORY "/Users/teograjqevci/dev/RayMarching_tool/build/_deps/glad-src")
endif()
file(MAKE_DIRECTORY
  "/Users/teograjqevci/dev/RayMarching_tool/build/_deps/glad-build"
  "/Users/teograjqevci/dev/RayMarching_tool/build/_deps/glad-subbuild/glad-populate-prefix"
  "/Users/teograjqevci/dev/RayMarching_tool/build/_deps/glad-subbuild/glad-populate-prefix/tmp"
  "/Users/teograjqevci/dev/RayMarching_tool/build/_deps/glad-subbuild/glad-populate-prefix/src/glad-populate-stamp"
  "/Users/teograjqevci/dev/RayMarching_tool/build/_deps/glad-subbuild/glad-populate-prefix/src"
  "/Users/teograjqevci/dev/RayMarching_tool/build/_deps/glad-subbuild/glad-populate-prefix/src/glad-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/teograjqevci/dev/RayMarching_tool/build/_deps/glad-subbuild/glad-populate-prefix/src/glad-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/teograjqevci/dev/RayMarching_tool/build/_deps/glad-subbuild/glad-populate-prefix/src/glad-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
