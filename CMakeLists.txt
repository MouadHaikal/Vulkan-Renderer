cmake_minimum_required(VERSION 3.10)
project(VulkanApp)

# Compiler settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS True) # For clangd lsp

# Output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Dependencies
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

# Source files
file(GLOB_RECURSE SRC_FILES src/*.cpp)


add_executable(VulkanApp ${SRC_FILES})

# Include directories
target_include_directories(VulkanApp PRIVATE include vendor)

# Link libraries
target_link_libraries(VulkanApp PRIVATE 
    Vulkan::Vulkan 
    glfw 
    glm::glm
)
