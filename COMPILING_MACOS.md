== Compiling on macOS (Apple Silicon / arm64) ==

This is a native macOS port of The Dark Mod engine. It has been tested on
Apple Silicon (M1/M2/M3/M4 family, arm64) running macOS 26 (Sequoia+).
It should also build on Intel Macs (x86_64) with minor adjustments,
but that configuration is untested.

Prerequisites:
  - Xcode Command Line Tools (clang, with C++17 support)
  - CMake >= 3.16
  - Homebrew packages: zlib minizip curl libvorbis libjpeg-turbo libpng ffmpeg
      brew install zlib minizip curl libvorbis libjpeg-turbo libpng ffmpeg

Directory layout (matches the Linux/Windows conventions):
  [base_tdm_path]/darkmod       <-- darkmod game location (thedarkmod.x64)
  [base_tdm_path]/darkmod_src   <-- source code location

== Step 1: Bootstrap third-party dependency Tracy ==

The official builds obtain Tracy 0.13.1 via a conan package with two
modifications (RecreateQueries patch + qgl* renaming, see
ThirdParty/custom/tracy/conanfile.py). This port adds a third change:
the upstream "#if !defined TRACY_ENABLE || defined __APPLE__" gate is
removed so the OpenGL backend compiles on macOS (GL timer queries work
in macOS GL 4.1 core contexts).

Run the bootstrap script from the repo root (installs to ~/tdm-deps):

  ./sys/cmake/setup_macos_tracy.sh

It downloads the tarball, verifies its SHA256, applies the same patches
the conan recipe applies, and writes a small CMake shim package.

== Step 2: Configure and build ==

From the source root, create a build directory and configure:

  cmake -B build -DCMAKE_BUILD_TYPE=Release

This picks up Homebrew dependencies via sys/cmake/homebrew_shims.cmake
(pkg-config-based shims for conan-style imported targets) and the local
Tracy package from ~/tdm-deps.

Then build:

  cmake --build build -j

The build outputs the binaries to ../darkmod relative to the source
directory (see the top of this file for the recommended layout), so the
game executable lands at [base_tdm_path]/darkmod/thedarkmod.x64.

== Step 3: Run ==

  cd [base_tdm_path]/darkmod && ./thedarkmod.x64

Notes:
  - The engine reports "unsupported CPU" and falls back to idSIMD_Generic
    on arm64. This is intentional and harmless; NEON SIMD paths are not
    implemented in the engine.
  - macOS provides OpenGL 4.1 (Metal-backed). A few optional GL
    extensions are missing (GL_ARB_buffer_storage, GL_ARB_multi_draw_indirect,
    GL_KHR_debug, etc.); the engine has fallback paths for all of them.
  - The optional tools roqvq (RoQ video encoder) is not built on macOS:
    it requires libjpeg private headers (jpegint.h) which Homebrew does
    not ship. A stub is linked instead (tools/compilers/roqvq_stub.cpp).
