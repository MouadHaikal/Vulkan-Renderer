#!/bin/zsh
set -e  # Exit immediately if a command exits with a non-zero status

if [ ! -d "build" ]; then
  mkdir build
fi

cd build
cmake ..
cmake --build .

cd ../bin
./VulkanApp
