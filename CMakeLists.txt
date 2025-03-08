cmake_minimum_required(VERSION 3.14)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # For clangd lsp
project(VulkanApp)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set paths
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Find Vulkan
find_package(Vulkan REQUIRED)

# Find GLFW
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW REQUIRED glfw3)

# Include directories
include_directories(include)

# Add source files
file(GLOB_RECURSE SRC_FILES src/*.cpp)

# Add the executable
add_executable(VulkanApp ${SRC_FILES})

# Include directories
target_include_directories(VulkanApp PRIVATE ${GLFW_INCLUDE_DIRS})

# Link Libraries
target_link_libraries(VulkanApp PRIVATE Vulkan::Vulkan ${GLFW_LIBRARIES})

# Compiler flags
target_compile_definitions(VulkanApp PRIVATE ${GLFW_CFLAGS_OTHER})
