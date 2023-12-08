# bellhop-opengl

Very rough draft

## Dependencies
- OpenGL 3.3
- GLFW

## Build

### Prerequisites
In order for `bellhopgl` to work, please make sure you have the following dependencies installed:
- OpenGL
- GLFW
- GLM
- FreeType2

To install these on Ubuntu 22.04, run
```bash
sudo apt update
sudo apt install mesa-common-dev libglfw3-dev libglm-dev libfreetype6-dev
```

Prepare your build environment by running the following at the project root
```bash
mkdir build
cd build 
```

In CMakeLists.txt in the root directory, locate the following line:
```
option(BHC_ENABLE_CUDA "Build CUDA version in addition to C++ version" OFF)
```

Change `OFF` to `ON` if on a system with an NVIDIA graphics card. Save the file and finish the rest of the build process:

```
cmake ..
make -j
```

## Run
All binaries are located in the `bin` folder in the root directory. To run `bellhopgl` specifically, you must be in the bin directory:
```bash
cd bin
./bellhopgl
```