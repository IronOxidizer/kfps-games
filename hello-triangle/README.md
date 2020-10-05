# hello-triangle

This demo could be be more efficient if the code separated into reasonable functions, however, I wanted it to be as simple as possible to help new Vulkan users. No functions (readable top down), no fancy tricks, minimal code.

```
gn gen out && cd out
clear && ninja -C . && ./hello-triangle && echo $?
```
![hello-triangle](https://user-images.githubusercontent.com/60191958/94946985-ee116180-04aa-11eb-8d91-059c1f29fcf8.png)

## Benchmarks

Optimization results (1000000 frame average)

- Debug: 23.6kfps
- No Debug: 34.7kfps
- No Debug + Compile Optimize: 35.5kfps

## Dependencies

#### Runtime

- libc
- libglfw
- libvulkan

#### Compile

- gn
- ninja
- clang
- glslc (usually part of shaderc or VulkanSDK package)
- libc headers
- vulkan headers
- glfw headers

### Compile on Windows

Have the following in `PATH`:
- gn
- ninja
- clang
- glslc

Have the following in `src`:
- vulkan headers folder
- GLFW header folder
- glfw3dll.lib

Have the following in `out`:
- glfw3.dll in out