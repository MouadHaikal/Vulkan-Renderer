#!/bin/zsh
set -e  # Exit immediately if a command exits with a non-zero status

# Ensure the build directory exists
if [ ! -d "build" ]; then
  mkdir build
fi

# Change to the build directory, configure and build
cd build
cmake ..
cmake --build .

# Return to the project root then enter the bin directory to run the app
cd ../bin
./VulkanApp
