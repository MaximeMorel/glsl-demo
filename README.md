glsl-demo
=========

Simple C clone of shadertoy, to make glsl demo.

How to use :

```
./glsl-demo
```

It will automatically scan for *.fs file in the same folder.

 * Use n for next shader, b for previous shader
 * p to pause time
 * +- to increase, decrease timestep
 * w to toggle fullscreen
 * If the active shader .fs file is modified, it will recompile the shader
 * You can almost copy/paste shadertoy shaders to test (mostly the one wihtout attachements, others needs some tweaking)

How to compile :

```
gcc glsl.c -lGL -lglut -lGLEW -o glsl-demo && ./glsl-demo
```
