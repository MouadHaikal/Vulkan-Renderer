cmake_minimum_required(VERSION 3.10)
project(VulkanApp)

# Compiler settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS True) # For LSP

# Output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

# Dependencies
find_package(Vulkan REQUIRED)

# Source files
file(GLOB_RECURSE SRC_FILES src/*.cpp)

# Add GLFW from source (vendored)
add_subdirectory(${PROJECT_SOURCE_DIR}/vendor/glfw)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Disable GLFW examples" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "Disable GLFW tests" FORCE)


add_executable(VulkanApp ${SRC_FILES})

# Include directories
target_include_directories(VulkanApp PRIVATE include vendor)

# Link libraries
target_link_libraries(VulkanApp PRIVATE 
    Vulkan::Vulkan 
    glfw 
)
