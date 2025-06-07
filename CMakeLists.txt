cmake_minimum_required(VERSION 3.20)
project(VulkanRenderer)

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


# Add GLFW from source
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Disable GLFW examples" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "Disable GLFW tests" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "Disable GLFW documentation" FORCE)
add_subdirectory(${PROJECT_SOURCE_DIR}/vendor/glfw)

# Add ASSIMP from source
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_VIEW OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_DOCS OFF CACHE BOOL "Disable Assimp documentation" FORCE)
add_subdirectory(${PROJECT_SOURCE_DIR}/vendor/assimp)


add_executable(VulkanRenderer ${SRC_FILES})

# Include directories
target_include_directories(VulkanRenderer PRIVATE include vendor)

# Link libraries
target_link_libraries(VulkanRenderer PRIVATE 
    Vulkan::Vulkan 
    glfw 
    assimp
)
