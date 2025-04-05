#!/bin/sh
set -e

mkdir -p build
cd build

cmake ..
cmake --build .

cd ../bin
./VulkanApp
