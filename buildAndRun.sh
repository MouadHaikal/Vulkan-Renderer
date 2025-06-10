#!/bin/sh
set -e

cd shaders
./compile.sh 
cd ..

mkdir -p build
cd build

cmake ..
cmake --build .

cd ../bin
./VulkanRenderer
