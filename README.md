# bellhop-opengl

Very rough draft

## Dependencies
- OpenGL 3.3
- GLFW

## Build
```bash
mkdir bld
g++ -o bld/test *.cpp includes/imgui/*.cpp *.c -lglfw -lGL -lm -I./includes
```

## Run
```bash
./bld/test
```