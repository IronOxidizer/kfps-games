# 10000fps-games
Games are slow, I want to know why. If there's no reason why, I will make them fast. Porting games to 10000fps.

Inspired by [Mincraft with C++ and Vulkan](https://vazgriz.com/312/creating-minecraft-in-one-week-with-c-and-vulkan-week-3/). Yeah, he got [Minecraft running at 2000+fps](https://vazgriz.com/wp-content/uploads/2020/06/block_selection-1.png)...

## Technologies

- critical metric: Runtime metric, or compile/dependency metric if no runtime exists

Criteria in order of importance, ranked from best (1) to worst (n):
- language: The native language of that technology, what itself is built with
- speed: Relative critical performance and execution time
- weight: Relative critical size
- portability: Relative ease of building on multiple operating systems
- difficulty: Relative ease of implementation for maximum performance

Final choice will be the technology that is **bolded**

### Rendering APIs

|                  | Language | Speed | Weight | Portability | Difficulty |
|------------------|----------|-------|--------|-------------|------------|
| Immediate OpenGL | C        | 3     | 1      | 1           | 1          |
| Retained OpenGL  | C        | 2     | 3      | 2           | 2          |
| **Vulkan**       | C        | 1     | 2      | 3           | 3          |

The goal of this project is to maximize performance, as such, Vulkan is the best rendering API to achieve that goal regardless of it's downsides. Its low CPU overhead, multithread compatability, and fine-grained control over the GPU makes it possible to leverage as much of a system's hardware as possible for maximum fps.

### Language

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

### libc

If I were only targeting Linux, this would be a section debating glibc, **musl libc**, and diet libc. However, this project aims to be as cross-platform as possible and the aforementioned libcs wouldn't easily work on Windows. As such, the system's native libc will be used and dynamically linked to at runtime.

If you're interested, [here's some comparisons between the libcs](https://www.etalabs.net/compare_libcs.html) and [here's a thread regarding why it's probably a bad idea to link statically libc anyways](https://stackoverflow.com/a/57478728).

### Compiler

|           | Language | Speed | Weight | Portability |
|-----------|----------|-------|--------|-------------|
| GCC       | C++      | 1     | 3      | 2           |
| **Clang** | C++      | 2     | 2      | 1           |
| TCC       | C        | 3     | 1      | 3           |

Unfortunately, GCC has been built with C++ ever since 2008. The prospect of having a self-hosting compiler using only C would have made this decision easy, however, the added complexity of C++ makes GCC much less attractive.

TCC was never a real consideration since it's no longer maintained, has very limited portability, and has much worse performance than the other two. However, it's my favorite C compiler in the sense that it's impossibly tiny, insanely fast at compiling, and self-hosting.

Clang has the major advantage of being extremely portable and can cross compile without needing to compile a separate compiler. Additionally, it has better native tooling on Windows and generally provides a better debugging experience along with generally better compile times. Although there is a speed difference of the output, it's marginal at less than 5%.

### Build System

|           | Language | Speed | Weight | Portability |
|-----------|----------|-------|--------|-------------|
| Make      | C        | 3     | 1      | 3           |
| CMake     | C++      | 2     | 3      | 1           |
| **Ninja** | C++      | 1     | 2      | 2           |

Although Make is the most common build system for classical or small C projects, it's fairly slow while being much less portable than the alternatives

CMake has been the industry standard for cross-platform C and C++ projects. Its main disadvantage is its huge tooling and its support for many backends which makes it very bloated and complex, especially for small, simple C projects.

Ninja isn't a build system on its own, but rather a backend build system which expects a frontend to generate build files for it, specific for the system it's running on. This makes it much faster than the others as it can provide many optimizations based on the context and environment. Additionally it's small and built from the ground up with modern C++ to build projects as fast as possible.

### Ninja Frontend

|        | Language | Speed | Weight | Portability | Difficulty |
|--------|----------|-------|--------|-------------|------------|
| CMake  | C++      | 2     | 4      | 1           | 4          |
| Meson  | Python   | 3     | 2      | 2           | 1          |
| GYP    | Python   | 4     | 3      | 4           | 3          |
| **gn** | C        | 1     | 1      | 3           | 2          |

Because CMake is written in C++ and has been the industry standard for many years, it's speed and portability are among the best. However, its accumulation of legacy targets and decades of bloat are the reason why it's so heavy and has an outdated syntax that makes it hard to use.

Meson and Gyp are decent alternatives but they both suffer from being built with Python. This reduces performance significantly and makes them much heavier as Python in itself is a massive dependency.

gn is the opposite of CMake, it's relatively new and only targets Ninja. Its syntax is a mix of Meson and GYP which makes it simple but also very expressive when it needs to be. On top of that it's built with C and very small which makes it the perfect candidate.

### Windowing and Input

|          | Language | Weight | Portability |
|----------|----------|--------|-------------|
| SDL2     | C        | 3      | 1           |
| SFML     | C++      | 2      | 3           |
| **GLFW** | C        | 1      | 2           |

SDL2 has been one of the most influential toolkits in open-source gaming as it brings portability and features to completely different level compared to its contemporaries (there's a reason valve choose it as the base for their games). Unfortunately, this comes at the cost of weight which makes SDL2 massively bloated. We wouldn't be using many of the features (built-in 2D renderer and audio) anyways so they'd end up being a waste of space and complexity.

SFML is between GLFW and SDL2 in terms of features and complexity. Unfortunately it's less portable and uses C++ natively.

GLFW is one of the lightest windowing toolkit. It provides simple input handling and is almost as portable as SDL2 (can even target mobile). It has all the features I need and none of the ones I don't.

### Audio and Networking

I am not interested in either of these. However, if someone submitted a PR that implement them in a way that does not impact performance, I wouldn't be opposed to accepting it.

If I did implement audio, it would probably use [OpenAL](https://openal.org/) with [Opus](https://opus-codec.org/).

As for networking, I would probably use a cross-platform and async interface to UDP like [libuv](https://libuv.org/).
