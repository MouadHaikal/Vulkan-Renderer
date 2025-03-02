cmake_minimum_required(VERSION 3.10)
project(VulkanApp)

add_executable(VulkanApp src/main.cpp)  # Declare the target
target_include_directories(VulkanApp PRIVATE ${CMAKE_SOURCE_DIR}/include)
