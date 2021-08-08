import os, shutil
from conans import ConanFile, CMake, tools


class DevilConan(ConanFile):
    name = "devil"
    version = "1.7.8"
    license = "LGPL-2.1"
    description = "A full-featured cross-platform image library"
    homepage = "http://openil.sourceforge.net"
    topics = ("image")

    author = "Stepan Gatilov stgatilov@gmail.com"

    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False]}
    default_options = "shared=False"
    generators = (
        "cmake",                # produces conanbuildinfo.cmake with generic cmake configuration
        "cmake_find_package",   # produces findXXX.cmake for find_package to properly find dependencies
    )
    requires = (
        "libjpeg/9c@bincrafters/stable",
        "libpng/1.6.34@bincrafters/stable",
    )
    source_subfolder = "source_subfolder"
    exports_sources = ["CMakeLists.txt", "config.h"]


    def source(self):
        # Download and extract tag tarball from SourceForge
        source_url = "https://sourceforge.net/projects/openil/files/DevIL"
        tools.get("{0}/{1}/DevIL-{1}.tar.gz".format(source_url, self.version))
        extracted_dir = "devil-" + self.version
        os.rename(extracted_dir, self.source_subfolder)
        shutil.copy("CMakeLists.txt", os.path.join(self.source_subfolder, "src-IL", "CMakeLists.txt"))
        # note: custom config --- set manually
        shutil.copy("config.h", os.path.join(self.source_subfolder, "include/IL", "config.h"))
        # patch function used for less-than-8-bit grayscale images: it was deprecated and removed in new libpng
        tools.replace_in_file(os.path.join(self.source_subfolder, "src-IL/src", "il_icon.c"), "png_set_gray_1_2_4_to_8", "png_set_expand_gray_1_2_4_to_8")
        tools.replace_in_file(os.path.join(self.source_subfolder, "src-IL/src", "il_png.c"), "png_set_gray_1_2_4_to_8", "png_set_expand_gray_1_2_4_to_8")
        # stgatilov: remove __LCC__ define to fix compilation on Elbrus
        tools.replace_in_file(os.path.join(self.source_subfolder, "include/IL", "il.h"), "defined(__LCC__)", "0/*defined(__LCC__)*/")


    def build(self):
        cmake = CMake(self)
        cmake.definitions["BUILD_SHARED_LIBS"] = self.options.shared
        if self.settings.compiler == 'gcc':
            cmake.definitions["CMAKE_C_FLAGS"] = "-fgnu89-inline"
        cmake.configure(source_folder = os.path.join(self.source_subfolder, "src-IL"))
        cmake.build()

    def package(self):
        self.copy("il.h", dst="include/IL", src = os.path.join(self.source_subfolder, "include/IL"))
        self.copy("*DevIL.lib", dst="lib", keep_path=False)
        self.copy("*libIL.a", dst="lib", keep_path=False)
        self.copy("*.dll", dst="bin", keep_path=False)
        self.copy("*.so", dst="lib", keep_path=False)
        self.copy("*.dylib", dst="lib", keep_path=False)
        self.copy("*COPYING", dst="licenses", keep_path=False)

    def package_info(self):
        self.cpp_info.libs = ["DevIL"]

