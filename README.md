# kfps-games

Games are slow, I want to know why. If there's no reason why, I will make them fast. Porting games to kfps.

Games so well optimized, they're measured in kfps (kiloframes per second). The goal is to create a set of demo applications as a reference for programmers and game devs on how to make high performance graphics applications. The hope is to set a benchmark for what is possible with Vulkan, and to challenge people on the idea of "acceptable performance" by raising the bar. All demos will be constrained to 768MB of RAM and 256MB of VRAM.

Inspired by [Mincraft with C++ and Vulkan](https://vazgriz.com/312/creating-minecraft-in-one-week-with-c-and-vulkan-week-3/). Yeah, he got [Minecraft running at 2kfps](https://vazgriz.com/wp-content/uploads/2020/06/block_selection-1.png)...

Disclaimer: [Extremely high framerates (+5000fps)](https://en.wikipedia.org/wiki/Electromagnetically_excited_acoustic_noise_and_vibration) can produce coil whine. Test at your own risk.

## Milestone Demos

- [x] [hello-triangle](hello-triangle/README.md) ~ (45.5kfps, 13kB executable)
- [ ] pong
- [ ] hello-cube
- [ ] voxel-game

## Build Instructions

It's prefered to download dependencies and dev packages from your package manager, but if they're not available, you can find them here:

- `gn` https://gn.googlesource.com/gn/
- `ninja` https://github.com/ninja-build/ninja/releases
- `clang` https://releases.llvm.org/download.html
- `glslc` https://vulkan.lunarg.com/sdk/home
- `vulkan headers` https://vulkan.lunarg.com/sdk/home
- `glfw headers` https://www.glfw.org/download
- `vulkan dlls` https://vulkan.lunarg.com/sdk/home
- `glfw dlls` https://www.glfw.org/download

On windows, place the `vulkan` + `GLFW` include folders and `glfw3dll.lib` inside `src`.

To run on windows, place `glfw3.dll` in the same library as the executable.

## Technologies

TODO: Use GWSL instead of GLSL for WebGPU support and finer control over SPIR-V generation

- critical metric: Runtime metric, or compile/dependency metric if no runtime exists

Criteria in order of importance, ranked from best (1) to worst (n):
- language: The native language of that technology, what itself is built with
- speed: Relative critical performance and execution time
- weight: Relative critical size
- portability: Relative ease of building on multiple operating systems
- difficulty: Relative ease of implementation for maximum performance

Final choice will be the technology that is **bolded**

#### Rendering APIs

|                  | Language | Speed | Weight | Portability | Difficulty |
|------------------|----------|-------|--------|-------------|------------|
| Immediate OpenGL | C        | 3     | 1      | 1           | 1          |
| Retained OpenGL  | C        | 2     | 3      | 2           | 2          |
| **Vulkan**       | C        | 1     | 2      | 3           | 3          |

The goal of this project is to maximize performance, as such, Vulkan is the best rendering API to achieve that goal regardless of it's downsides. Its low CPU overhead, multithread compatibility, and fine-grained control over the GPU makes it possible to leverage as much of a system's hardware as possible for maximum fps.

#### Language

|       | Speed | Weight | Portability | Difficulty |
|-------|-------|--------|-------------|------------|
| **C** | 2     | 1      | 1           | 1          |
| C++   | 1     | 2      | 2           | 4          |
| Go    | 4     | 4      | 4           | 2          |
| Rust  | 3     | 3      | 3           | 3          |

A small explanation before elaborating on my selection. C and Rust can be just as fast as C++ when well written but there are a few things that make this difficult.

C++'s meta-programming system (particularly templates) often give it a performance advantage over C. The same implementation can be done in C but it would require significantly more work.

Rust also has a very powerful meta-programming system with it's procedural macros, but often times, getting high performance out of Rust means using unsafe which negates some of it's advantages. Additionally, Rust does not always have native bindings to core libraries which will reduce performance, especially if it can't be statically compiled.

Go is automatically garbage collected and binds much less cleanly to Vulkan.

As for my selection of C, Vulkan (and many of the technologies I evaluate later) uses C for their implementation. As such, building will be much simpler and no additional bindings (which may incur an overhead) are need. Not having C++ or Rust's extensive meta-programming features will mean I will need to think much deeper about the implementation to ensure that it is as high performance as possible.

#### libc

If I were only targeting Linux, this would be a section debating glibc, **musl libc**, and diet libc. However, this project aims to be as cross-platform as possible and the aforementioned libcs wouldn't easily work on Windows. As such, the system's native libc will be used and dynamically linked to at runtime.

If you're interested, [here's some comparisons between the libcs](https://www.etalabs.net/compare_libcs.html) and [here's a thread regarding why it's probably a bad idea to link statically libc anyways](https://stackoverflow.com/a/57478728).

#### Compiler

|           | Language | Speed | Weight | Portability |
|-----------|----------|-------|--------|-------------|
| GCC       | C++      | 1     | 3      | 2           |
| **Clang** | C++      | 2     | 2      | 1           |
| TCC       | C        | 3     | 1      | 3           |

Unfortunately, GCC has been built with C++ ever since 2008. The prospect of having a self-hosting compiler using only C would have made this decision easy, however, the added complexity of C++ makes GCC much less attractive.

TCC was never a real consideration since it's no longer maintained, has very limited portability, and has much worse performance than the other two. However, it's my favorite C compiler in the sense that it's impossibly tiny, insanely fast at compiling, and self-hosting.

Clang has the major advantage of being extremely portable and can cross compile without needing to compile a separate compiler. Additionally, it has better native tooling on Windows and generally provides a better debugging experience along with generally better compile times (as a result of its state of the art linker, lld). Although there is a speed difference of the output, it's marginal at less than 5%.

#### Build System

|           | Language | Speed | Weight | Portability |
|-----------|----------|-------|--------|-------------|
| Make      | C        | 3     | 1      | 3           |
| CMake     | C++      | 2     | 3      | 1           |
| **Ninja** | C++      | 1     | 2      | 2           |

Although Make is the most common build system for classical or small C projects, it's fairly slow while being much less portable than the alternatives

CMake has been the industry standard for cross-platform C and C++ projects. Its main disadvantage is its huge tooling and its support for many backends which makes it very bloated and complex, especially for small, simple C projects.

Ninja isn't a build system on its own, but rather a backend build system which expects a frontend to generate build files for it, specific for the system it's running on. This makes it much faster than the others as it can provide many optimizations based on the context and environment. Additionally it's small and built from the ground up with modern C++ to build projects as fast as possible.

#### Ninja Frontend

|        | Language | Speed | Weight | Portability | Difficulty |
|--------|----------|-------|--------|-------------|------------|
| CMake  | C++      | 2     | 4      | 1           | 4          |
| Meson  | Python   | 3     | 2      | 2           | 1          |
| GYP    | Python   | 4     | 3      | 4           | 3          |
| **gn** | C        | 1     | 1      | 3           | 2          |

Because CMake is written in C++ and has been the industry standard for many years, it's speed and portability are among the best. However, its accumulation of legacy targets and decades of bloat are the reason why it's so heavy and has an outdated syntax that makes it hard to use.

Meson and Gyp are decent alternatives but they both suffer from being built with Python. This reduces performance significantly and makes them much heavier as Python in itself is a massive dependency.

gn is the opposite of CMake, it's relatively new and only targets Ninja. Its syntax is a mix of Meson and GYP which makes it simple but also very expressive when it needs to be. On top of that it's built with C and very small which makes it the perfect candidate.

#### Windowing and Input

|          | Language | Weight | Portability |
|----------|----------|--------|-------------|
| SDL2     | C        | 3      | 1           |
| SFML     | C++      | 2      | 3           |
| **GLFW** | C        | 1      | 2           |

SDL2 has been one of the most influential toolkits in open-source gaming as it brings portability and features to completely different level compared to its contemporaries (there's a reason valve choose it as the base for their games). Unfortunately, this comes at the cost of weight which makes SDL2 massively bloated. We wouldn't be using many of the features (built-in 2D renderer and audio) anyways so they'd end up being a waste of space and complexity.

SFML is between GLFW and SDL2 in terms of features and complexity. Unfortunately it's less portable and uses C++ natively.

GLFW is one of the lightest windowing toolkit. It provides simple input handling and is almost as portable as SDL2 (can even target mobile). It has all the features I need and none of the ones I don't.

#### Audio and Networking

I am not interested in either of these. However, if someone submitted a PR that implement them in a way that does not impact performance, I wouldn't be opposed to accepting it.

If I did implement audio, it would probably use [OpenAL](https://openal.org/) with [Opus](https://opus-codec.org/).

As for networking, I would probably use a cross-platform and async interface to UDP like [libuv](https://libuv.org/).

## Techniques, Algorithms, and Patterns

#### Compositor Bypass

- GLFW will bypass the display compositor in full screen mode which removes an unnecessary memory copy. This reduces latency and resource overhead, generally improving performance.

- This is done via [_NET_WM_BYPASS_COMPOSITOR](https://www.glfw.org/docs/3.3/compat.html)

#### Vulkan Configuration

- Select a queue that supports both presentation and graphics, enabling `VK_SHARING_MODE_EXCLUSIVE` for better performance
- Use `VK_PRESENT_MODE_IMMEDIATE_KHR` to disable multi-frame buffering
- Set `clipped = VK_TRUE` to enable frame clipping, not needing to render things that are being obstructed
- Ignore the base alpha channel with `VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR` to improve performance
- Manual function pointer fetching is done because using `vulkan.h` uses indirection to point to the actual functions in the driver. By fetching the function pointer ourselves, we can improve performance by ~2% when calling them.

More tips here: https://developer.nvidia.com/blog/vulkan-dos-donts/

#### Greedy Meshing

- Cull before grouping or else mesh groups will not be as efficient as possible

#### Entity Component System

#### Multithreaded Rendering

#### Shaders

- On AMD systems, MESA ACO should be used for improved performance

#### Bit compression

- https://youtu.be/VQuN1RMEr1c
- Consider geometry shaders which can generate object vertices based on a single point.
- Fit vertex data cleanly into powers of 2.
- Keep in mind that video memory is aligned to 4 bytes, so 33 bits is effectively the same as 64 bits. Don't over optimize if it won't compress past a 4 byte threshold.
- Also consider storing data in a shader array then indexing to it in the vertex data for minimum memory usage.

#### Micro optimizations

- Leverage macros: Do as much as possible at compile time to reduce runtime computation and executable size.

- No VLAs: https://phoronix.com/scan.php?page=news_item&px=Linux-Kills-The-VLA

## Compile Optimizations

#### Dynamic Linking

- Although static linking can [improve performance](https://gcc.gnu.org/legacy-ml/gcc/2004-06/msg01956.html), there are too many issues with Vulkan driver interfaces changing for it to be viable. Also GLFW [no longer supports static linking](https://github.com/glfw/glfw/issues/1776) with the Vulkan loader. Dynamic linking can also provide memory usage advantages as dynamic libraries that are being used by many programs simultaneously only need to be loaded once. In addition, because the libraries are not statically linked, the final executable is smaller which can provide performance advantages.

#### Compile Flags

- https://clang.llvm.org/docs/CommandGuide/clang.html
- https://wiki.gentoo.org/wiki/Clang
- https://manpages.debian.org/experimental/lld-10/ld.lld-10.1.en.html

```sh
CFLAGS='-pipe -flto -march=native -mtune=native -mcpu=native -Ofast -fno-stack-protector -fvisibility=hidden -fno-pic -fno-pie'
LDFLAGS="-flto", "-fuse-ld=lld", "-Wl,-O2,--gc-sections,--as-needed,-s"
```

- [-pipe](https://stackoverflow.com/questions/1512933): Use more memory when compiling to avoid writing to disk and improve parallelization which improves compile speed.

- [-flto](https://gcc.gnu.org/wiki/LinkTimeOptimization): Enables link time optimization.

- [-march=native -mtune=native -mcpu=native](https://community.arm.com/developer/tools-software/tools/b/tools-software-ides-blog/posts/compiler-flags-across-architectures-march-mtune-and-mcpu): Optimizes code for the system's architecture and CPU.

- [-Ofast](https://www.phoronix.com/scan.php?page=article&item=gcc_47_optimizations): Optimize for improved runtime performance.

- [-fno-stack-protector](https://stackoverflow.com/questions/10712972): Disables stack protector which removes stack smashing/clashing checking and allows for a more efficient stack layout.

- [-fvisibility=hidden](https://gcc.gnu.org/wiki/Visibility): Lowers chance of symbol collision, reduces code size, enables more optimizations.

- [-fno-pic -fno-pie](https://eli.thegreenplace.net/2011/11/03/position-independent-code-pic-in-shared-libraries/): Symbols are kept in the same relative position to remove the need of address resolution at runtime which would otherwise potentially reduce performance.

Linker Flags:

- [-flto, -fuse-ld=lld](https://llvm.org/docs/LinkTimeOptimization.html): Same as previous flto but enables LTO for the linker using lld.

- [-O2](https://manpages.debian.org/experimental/lld-10/ld.lld-10.1.en.html#O): -O2 is the highest level of linker optimization for lld.

- [--gc-sections](https://manpages.debian.org/experimental/lld-10/ld.lld-10.1.en.html#-gc-sections):
Remove any unused code from the executable.

- [--as-needed](https://stackoverflow.com/questions/6411937#6412134): Disables linking for libraries that don't appear to be used, even when explicitly specified potentially reducing executable size.

- [-s](https://manpages.debian.org/experimental/lld-10/ld.lld-10.1.en.html#-strip-all): The same as `--strip-all`, strips all symbols reducing executable size.

Debugging:

```sh
-g3 -ggdb3 -Og -fvisibility=default
```

#### Strip

- Although many people don't bother stripping their binaries, it's an important part of improving performance. Modern CPU's are able to run as fast as they do because they don't need to always fetch data from RAM, but rather, they have the option to store and fetch data from its many caches. These caches are much smaller than RAM but are also much faster ([up to 500x RAM](https://www.quora.com/How-fast-can-the-L1-cache-of-a-CPU-reach?)). As such, it's important to try to fit as much of your code and data in the level 1 cache as possible, particularly when there is lots of context switching occurring. This is exactly why [kernels optimize for size](https://lwn.net/Articles/534735/) with -Os rather than for speed with -O3, because the performance gains of fitting the entire kernel in low-level cache can be massive.

## Validation Environment

- RPi4 1GB
- 256MB VRAM
- BuildRoot
- musl libc
- busybox
- experimental vulkan module
- mesa-git
- minimal 64bit kernel
- minimal-xorg
- TinyWM
- 1920x1080

.xinitrc:
```
sleep 3 && hello-triangle &
exec tinywm
```

Guides:
 - https://unix.stackexchange.com/questions/70931#306116

Why the Raspberry Pi 4?
- Very affordable
- Popular
- 64bit
- Vulkan
