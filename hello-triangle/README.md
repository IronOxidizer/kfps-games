# hello-triangle

This demo could be be more efficient if the code separated into reasonable functions, however, I wanted it to be as simple as possible to help new Vulkan users. No functions (readable top down), no fancy tricks, minimal code.

```
gn gen out && cd out
clear && ninja -C . && ./hello-triangle && echo $?
```
![hello-triangle](https://user-images.githubusercontent.com/60191958/94946985-ee116180-04aa-11eb-8d91-059c1f29fcf8.png)

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

- gn
- ninja
- clang
- glslc
- vulkan headers in src
- GLFW headers in src
- glfw3dll.lib in src
- glfw3.dll in out