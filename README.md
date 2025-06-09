# Vulkan Renderer

A real‑time rasterization renderer written from scratch in modern C++ with [Vulkan](https://www.vulkan.org/) providing a clean foundation for experimenting with advanced rendering techniques offering high performance, low-level control, and seamless portability across Linux and Windows.
<br/><br/>

## Table of Contents
+ [Features](#features)
    + [Normal Mapping](#normal-mapping)
    + [Asset Loading](#asset-loading)
        + [3D Models](#3d-models)
        + [Textures](#textures)
    + [Anti-aliasing](#anti-aliasing)
        + [MSAA](#msaa)
        + [Mipmapping](#mipmapping)
    + [Camera Controls](#camera-controls)
+ [Cloning](#cloning)
  + [Building](#building)
<br/><br/>

## Features
This is an active project with features continuously being added and refined.
<br/><br/>

## Normal Mapping
<img src="https://img.icons8.com/?size=100&id=d0hMePbLym7W&format=png&color=000000"  
    height="25"  
    style="vertical-align: middle;"  
    alt="WIP Icon" />  *Work In Progress...*
<p align="center">
    <img src="docs/screenshots/VertexNormals.png" width="600">
</p>
<br/><br/>

## Asset Loading
### 3D Models
The renderer uses the [Assimp](https://github.com/assimp/assimp) library to load 40+ 3D-file-formats. Once loaded, vertex and index data are extracted and used to create GPU buffers. These models are then smoothly integrated into the scene hierarchy, allowing complex models with multiple meshes and materials to be handled seamlessly.


<div align="center">
    <table>
        <tr>
            <td><img src="docs/screenshots/ModelLoadingWireframe.png"/></td>
            <td><img src="docs/screenshots/ModelLoading.png"/></td>
        </tr>
        <tr>
            <td>obj model - 450k triangles</td>
            <td>fbx model - 500k triangles</td>
        </tr>
    </table>
</div>

### Textures
Textures are loaded using [stb_image](https://github.com/nothings/stb/blob/master/stb_image.h). The renderer applies anisotropic filtering for higher-quality texture sampling at glancing angles and mipmaps are generated during upload to prevent aliasing and optimize performance. 
<br/><br/>

## Anti Aliasing
### MSAA
Multi-Sample Anti-Aliasing is implemented to smooth jagged edges by sampling each pixel multiple times and averaging the results. This technique significantly improves visual quality with minimal performance impact, and is integrated directly into the render pass and framebuffer pipeline.

<div align="center">
    <table>
        <tr>
            <td><img src="docs/screenshots/NoAA.png"/></td>
            <td><img src="docs/screenshots/MSAAx16.png"/></td>
        </tr>
        <tr>
            <td>No anti aliasing</td>
            <td>16x MSAA</td>
        </tr>
    </table>
</div>

### Mipmapping
Mipmaps are procedurally generated for each texture improving performance and reducing aliasing during minification. Mipmap levels are selected dynamically based on texture resolution.

<div align="center">
    <table>
        <tr>
            <td><img src="docs/screenshots/NoMipmapping.png" width="600"/></td>
            <td><img src="docs/screenshots/Mipmapping.png" width="600"/></td>
        </tr>
        <tr>
            <td>No mipmapping (Moiré patterns)</td>
            <td>Using mipmaps</td>
        </tr>
    </table>
</div>
<br/><br/>

## Camera Controls
The renderer implements real-time user-controlled camera movement using [GLFW](https://github.com/glfw/glfw) for input handling. The camera supports first-person navigation (WASD) and free-look via mouse movement. Input events are processed each frame and translated into position/orientation updates, enabling intuitive scene exploration.

<div align="center">
    <table>
        <tr>
            <td><img src="docs/recordings/CameraControls.gif"/></td>
        </tr>
        <tr>
            <td>Camera Controls Demo</td>
        </tr>
    </table>
</div>
<br/><br/>

## Cloning
This repository contains submodules for external dependencies. Clone recursively to ensure everything is fetched properly:
```
git clone --recursive https://github.com/MouadHaikal/Vulkan-Renderer
```
### Building
<img src="https://img.icons8.com/?size=100&id=d0hMePbLym7W&format=png&color=000000"  
    height="25"  
    style="vertical-align: middle;"  
    alt="WIP Icon" />  *Work In Progress...*
