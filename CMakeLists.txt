cmake_minimum_required(VERSION 3.20)
project(VulkanRenderer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS True) # For LSP

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

find_package(Vulkan REQUIRED)

file(GLOB_RECURSE SRC_FILES src/*.cpp)


# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(${PROJECT_SOURCE_DIR}/vendor/glfw)


# Assimp
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_VIEW OFF CACHE BOOL "" FORCE)
add_subdirectory(${PROJECT_SOURCE_DIR}/vendor/assimp)


# ImGui
set(IMGUI_DIR ${PROJECT_SOURCE_DIR}/vendor/imgui)
file(GLOB IMGUI_SOURCES  "${IMGUI_DIR}/*.cpp")
list(APPEND IMGUI_SOURCES
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp 
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp 
    ${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp
)


# fmt
add_subdirectory(${PROJECT_SOURCE_DIR}/vendor/fmt)


add_executable(VulkanRenderer ${SRC_FILES} ${IMGUI_SOURCES})

target_include_directories(VulkanRenderer PRIVATE 
    include 
    vendor
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${IMGUI_DIR}/misc/cpp
)

target_link_libraries(VulkanRenderer PRIVATE 
    Vulkan::Vulkan 
    glfw 
    assimp
    fmt::fmt
)
