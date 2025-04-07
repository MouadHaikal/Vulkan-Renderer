# Arch Linux Build Guide

This repository contains a Vulkan-based application. Follow the instructions below to build and run the project from source on Arch Linux.


## Prerequisites

Before building the project, you need to install the following dependencies:

1. **Vulkan SDK**:
   ```sh
   sudo pacman -S vulkan-devel
   ```

2. **GLFW Dependencies**:
   The project builds GLFW from source internally. Install these dependencies:
   ```sh
   sudo pacman -S libgl libxkbcommon cmake doxygen extra-cmake-modules libxcursor libxi libxinerama libxrandr mesa wayland-protocols
   ```

## Building and Running

Once you have installed all the required dependencies, you can build and run the project using the provided script:

1. Clone this repository:
   ```sh
   git clone https://github.com/MouadHaikal/VulkanApp
   cd VulkanApp
   ```

2. Make the build script executable (if it isn't already):
   ```sh
   chmod +x buildAndRun.sh
   ```

3. Execute the build script:
   ```sh
   ./buildAndRun.sh
   ```

The script will handle building the project (including GLFW) and running the application.
