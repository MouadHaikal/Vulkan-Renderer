mkdir build
cd build

cmake ..
cmake --build . --config Release

cd ..\bin
VulkanRenderer.exe
