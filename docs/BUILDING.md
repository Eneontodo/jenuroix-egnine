# Building Jenuroix Engine

This guide covers building the engine and all examples from source on
Linux, macOS, and Windows.

---

## Requirements

| Requirement | Minimum | Notes |
|---|---|---|
| C++ compiler | GCC 11 / Clang 14 / MSVC 2022 | C++17 required |
| CMake | 3.21 | |
| OpenGL | 3.3 core | Provided by GPU driver |
| GLFW | 3.3 | Can be installed via package manager |
| Python | 3.8 | Optional — only for build helper scripts |

Optional features require additional libraries:

| Feature | Library | How to obtain |
|---|---|---|
| Physics | [Jolt Physics](https://github.com/jrouwe/JoltPhysics) | Build from source or vcpkg |
| Audio | [miniaudio](https://miniaud.io/) | Drop `miniaudio.h` into `src/` |

---

## Linux

```bash
# Install system dependencies (Ubuntu / Debian)
sudo apt install build-essential cmake libglfw3-dev libgl-dev

# Clone
git clone https://github.com/your-org/jenuroix-engine.git
cd jenuroix-engine

# Bootstrap (configures + builds)
chmod +x bootstrap.sh
./bootstrap.sh

# Or manually
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

Binaries are placed in `build/bin/`.

---

## macOS

```bash
# Install Homebrew dependencies
brew install cmake glfw

# Clone + build (same as Linux)
git clone https://github.com/your-org/jenuroix-engine.git
cd jenuroix-engine
./bootstrap.sh
```

> **Apple Silicon:** the engine compiles natively for arm64. Make sure your
> GLFW installation matches your target architecture.

---

## Windows

**Using the bootstrap batch file (recommended)**

```bat
git clone https://github.com/your-org/jenuroix-engine.git
cd jenuroix-engine
bootstrap.bat
```

This runs CMake with the Visual Studio generator and builds in Release mode.

**Manual (Visual Studio)**

```bat
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

**Manual (MSYS2 / MinGW)**

```bat
cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

---

## CMake Options

| Option | Default | Description |
|---|---|---|
| `ENGINE_BUILD_EXAMPLES` | `ON` | Build all example binaries |
| `ENGINE_ENABLE_PHYSICS` | `AUTO` | Link Jolt if found, stub otherwise |
| `ENGINE_ENABLE_AUDIO` | `AUTO` | Link miniaudio if `miniaudio.h` is found in `src/` |
| `CMAKE_BUILD_TYPE` | `Release` | `Debug` adds debug symbols and disables optimisations |

Example — build with physics enabled, skip audio:

```bash
cmake -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DENGINE_ENABLE_PHYSICS=ON \
  -DENGINE_ENABLE_AUDIO=OFF
cmake --build build --parallel
```

---

## Adding Jolt Physics

1. Clone Jolt into the repo root or a sibling directory:
   ```bash
   git clone https://github.com/jrouwe/JoltPhysics.git extern/JoltPhysics
   ```
2. Configure with the path:
   ```bash
   cmake -B build -DJOLT_SOURCE_DIR=extern/JoltPhysics
   ```
3. Examples 15 and 18 will now build with real physics. Without Jolt, a
   stub implementation is used so all other examples still compile.

---

## Adding miniaudio

1. Download the single-header file from <https://miniaud.io/>:
   ```bash
   curl -o src/miniaudio.h https://raw.githubusercontent.com/mackron/miniaudio/master/miniaudio.h
   ```
2. Reconfigure CMake — it detects `miniaudio.h` automatically.
3. Examples 16 and 18 will use real audio output.

---

## Troubleshooting

**"Cannot find GLFW"**  
Set `GLFW_DIR` in CMake to your GLFW installation directory:
```bash
cmake -B build -DGLFW_DIR=/usr/local/lib/cmake/glfw3
```

**"OpenGL not found" on Linux**  
Install the Mesa development package:
```bash
sudo apt install libgles2-mesa-dev
```

**White / black window on first launch**  
Check that your GPU driver supports OpenGL 3.3 core. On virtual machines,
try forcing software rendering: `LIBGL_ALWAYS_SOFTWARE=1 ./build/bin/01_hello`.

**Linker errors with Jolt on Windows**  
Make sure Jolt was built with the same runtime library (`/MD` vs `/MDd`)
as the engine. Pass `-DUSE_STATIC_MSVC_RUNTIME_LIBRARY=OFF` to the Jolt
CMake configure step.
