from conans import ConanFile, CMake, tools
import os


class TracyConan(ConanFile):
    name = "tracy"
    version = "0.7.8"
    license = "BSD-3-Clause"
    author = "Stepan Gatilov stgatilov@gmail.com"
    description = "A real time, nanosecond resolution, remote telemetry, hybrid frame and sampling profiler for games and other applications."
    topics = ("profiler", "trace")
    exports_sources = ["patches/*"]

    def source(self):
        # Download and extract tag tarball from github
        source_url = "https://github.com/wolfpld/tracy"
        tools.get("{0}/archive/refs/tags/v{1}.tar.gz".format(source_url, self.version))
        extracted_dir = "tracy-" + self.version
        os.rename(extracted_dir, "fullsource")

    def build(self):
        # add RecreateQueries method
        tools.patch(base_path = "fullsource", patch_file = "patches/RecreateQueries.patch")
        # replace glXXX with qglXXX
        for prefix in ['glGet', 'glGen', 'glQuery']:
            tools.replace_in_file("fullsource/TracyOpenGL.hpp", prefix, 'q' + prefix)

    def package(self):
        for dirname in ['client', 'common', 'libbacktrace']:
            self.copy("{0}/*.h".format(dirname), dst="include", src="fullsource")
            self.copy("{0}/*.hpp".format(dirname), dst="include", src="fullsource")
            self.copy("{0}/*.cpp".format(dirname), dst="src", src="fullsource")
        for filename in ['Tracy.hpp', 'TracyC.h', 'TracyOpenGL.hpp']:
            self.copy(filename, dst="include", src="fullsource")
        for filename in ['TracyClient.cpp']:
            self.copy(filename, dst="src", src="fullsource")
        self.copy("LICENSE", dst="licenses", src="fullsource")
        self.copy("libbacktrace/LICENSE", dst="licenses", src="fullsource")
