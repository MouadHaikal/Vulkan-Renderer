#!/bin/sh
set -e

cd shaders
./compile.sh 
cd ..

mkdir -p build
cd build

cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .

cd ../bin
./VulkanRenderer
