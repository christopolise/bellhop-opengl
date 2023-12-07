# bellhop-opengl

Very rough draft

## Dependencies
- OpenGL 3.3
- GLFW

## Build
```bash
mkdir bld
g++ -o bld/test *.cpp includes/imgui/*.cpp *.c -I./includes -I/usr/include/freetype2/ -lglfw -lGL -lfreetype
```

## Run
```bash
./bld/test
```