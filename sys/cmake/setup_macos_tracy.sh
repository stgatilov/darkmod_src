#!/usr/bin/env bash
# Bootstrap Tracy 0.13.1 for a macOS (Apple Silicon) build of The Dark Mod.
#
# The official builds use a conan-generated package of Tracy 0.13.1 with two
# modifications (see ThirdParty/custom/tracy/conanfile.py):
#   1. patches/0.10/RecreateQueries.patch  - adds GpuCtx::RecreateQueries()
#   2. glXXX -> qglXXX renaming in TracyOpenGL.hpp (game renames all GL calls)
# Plus one macOS-specific change needed for this port:
#   3. remove the "#if !defined TRACY_ENABLE || defined __APPLE__" gate which
#      hard-disables the OpenGL backend on Apple (GL timer queries work fine
#      in macOS GL 4.1 core contexts).
#
# This script downloads the upstream tarball, verifies its SHA256, applies the
# same modifications, and installs headers into:
#   ~/tdm-deps/tracy-0.13.1/public        (include root)
#   ~/tdm-deps/tracy-cmake/tracyConfig.cmake  (fake CMake package)
#
# Usage:  ./sys/cmake/setup_macos_tracy.sh
set -euo pipefail

TRACY_VERSION="0.13.1"
TRACY_URL="https://github.com/wolfpld/tracy/archive/refs/tags/v${TRACY_VERSION}.tar.gz"
TRACY_SHA256="d4efc50ebcb0bfcfdbba148995aeb75044c0d80f5d91223aebfaa8fa9e563d2b"

DEPS_ROOT="${HOME}/tdm-deps"
SRC_DIR="${DEPS_ROOT}/tracy-${TRACY_VERSION}"
CMAKE_DIR="${DEPS_ROOT}/tracy-cmake"
TMP_TARBALL="${DEPS_ROOT}/tracy-${TRACY_VERSION}.tar.gz"

if [ -f "${SRC_DIR}/public/tracy/TracyOpenGL.hpp" ]; then
	if grep -q "RecreateQueries" "${SRC_DIR}/public/tracy/TracyOpenGL.hpp"; then
		echo "Tracy ${TRACY_VERSION} already set up at ${SRC_DIR} - nothing to do."
		exit 0
	fi
fi

mkdir -p "${DEPS_ROOT}"

echo "Downloading Tracy ${TRACY_VERSION}..."
curl -sL -o "${TMP_TARBALL}" "${TRACY_URL}"

echo "Verifying SHA256..."
if command -v shasum >/dev/null 2>&1; then
	echo "${TRACY_SHA256}  ${TMP_TARBALL}" | shasum -a 256 -c -
else
	echo "${TRACY_SHA256}  ${TMP_TARBALL}" | sha256sum -c -
fi

echo "Extracting..."
rm -rf "${SRC_DIR}"
tar xzf "${TMP_TARBALL}" -C "${DEPS_ROOT}"
# tarball extracts to tracy-<version>/

PATCH_FILE="$(dirname "$0")/../../ThirdParty/custom/tracy/patches/0.10/RecreateQueries.patch"
echo "Applying RecreateQueries patch..."
patch -p1 -d "${SRC_DIR}/public" < "${PATCH_FILE}"

echo "Renaming gl* calls to qgl* in TracyOpenGL.hpp..."
# same replacements as ThirdParty/custom/tracy/conanfile.py build():
#   for prefix in glGet, glGen, glQuery: replace_in_file(TracyOpenGL.hpp, prefix, 'q' + prefix)
perl -pi -e 's/\bglGet(?!Ticks)\b/qglGet/g; s/\bglGen\b/qglGen/g; s/\bglQuery\b/qglQuery/g' \
	"${SRC_DIR}/public/tracy/TracyOpenGL.hpp"
perl -pi -e 's/\bglGenQueries\b/qglGenQueries/g; s/\bglGetQueryObjectiv\b/qglGetQueryObjectiv/g; s/\bglGetQueryObjectui64v\b/qglGetQueryObjectui64v/g; s/\bglQueryCounter\b/qglQueryCounter/g; s/\bglGetQueryiv\b/qglGetQueryiv/g; s/\bglGetInteger64v\b/qglGetInteger64v/g' \
	"${SRC_DIR}/public/tracy/TracyOpenGL.hpp"

echo "Enabling OpenGL backend on Apple (upstream disables it)..."
perl -pi -e 's/^#if !defined TRACY_ENABLE \|\| defined __APPLE__$/#if !defined TRACY_ENABLE/' \
	"${SRC_DIR}/public/tracy/TracyOpenGL.hpp"

echo "Creating CMake shim at ${CMAKE_DIR}..."
mkdir -p "${CMAKE_DIR}"
cat > "${CMAKE_DIR}/tracyConfig.cmake" <<EOF
# Fake CMake config for tracy (headers only; the client implementation is
# compiled into the game itself via framework/TracingEmbeddedSourceCode.cpp).
if(NOT TARGET tracy::tracy)
  add_library(tracy::tracy INTERFACE IMPORTED)
  set_target_properties(tracy::tracy PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${SRC_DIR}/public")
endif()
EOF

echo
echo "Done. Tracy ${TRACY_VERSION} ready at ${SRC_DIR}."
echo "Build TDM with (from repo root):"
echo "  cmake -B build -DCMAKE_BUILD_TYPE=Release"
echo "  cmake --build build -j"
