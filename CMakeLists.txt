cmake_minimum_required(VERSION 3.22)

project(UnityEditorDarkMode)
set(CMAKE_CXX_STANDARD 20)

# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# For multi-configuration tools (like Visual Studio), limit configurations to Release
set(CMAKE_CONFIGURATION_TYPES "Release" CACHE STRING "" FORCE)

if(UNIX)
    cmake_path(SET USERHOME NORMALIZE $ENV{HOME})
else()
    cmake_path(SET USERHOME NORMALIZE $ENV{USERPROFILE})
endif()

# DEPENDENCIES
include(FetchContent)
FetchContent_Populate(
  inipp
  URL https://github.com/mcmtroffaes/inipp/archive/c61e699682d3f1091209c2179f1d03f5fc593327.zip
  URL_HASH SHA256=1727e025c03e80db4f66925fa5e20e36943c2a2f94c6b141cb1f6f792a6cdca8
  EXCLUDE_FROM_ALL
)
cmake_path(SET INIPP_INCLUDE_DIRS NORMALIZE "${inipp_SOURCE_DIR}/inipp/")

if (MSVC)
  add_compile_options(/MP)
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)
  add_compile_definitions(NOMINMAX UNICODE)
endif()

add_library(${PROJECT_NAME} SHARED
    UnityEditorDarkMode.cpp
    UnityEditorDarkMode.def
)

target_include_directories(${PROJECT_NAME} PRIVATE ${INIPP_INCLUDE_DIRS})
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/DEF:\"${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.def\"")
