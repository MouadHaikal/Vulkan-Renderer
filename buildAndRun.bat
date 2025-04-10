mkdir build
cd build

:: Explicitly use Ninja (requires Ninja installed)
cmake -G "Ninja" ..
cmake --build . --config Release

cd ..\bin
VulkanRenderer.exe
