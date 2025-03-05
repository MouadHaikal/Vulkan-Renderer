cmake_minimum_required(VERSION 3.14)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # For clangd lsp
project(VulkanApp)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set paths
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Find Packages
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

# Include directories
include_directories(include)

# Add source files
file(GLOB_RECURSE SRC_FILES src/*.cpp)

# Add the executable
add_executable(VulkanApp ${SRC_FILES})

# Link Libraries
target_link_libraries(VulkanApp PRIVATE Vulkan::Vulkan glfw glm)
