import os
from conans import ConanFile, CMake, tools


class DevilConan(ConanFile):
    name = "devil"
    version = "1.8.0"
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


    def source(self):
        # Download and extract tag tarball from github
        source_url = "https://github.com/DentonW/DevIL"
        tools.get("{0}/archive/v{1}.tar.gz".format(source_url, self.version))
        extracted_dir = "DevIL-" + self.version
        os.rename(extracted_dir, self.source_subfolder)
        # This small hack might be useful to guarantee proper /MT /MD linkage
        main_cmakelists_path = self.source_subfolder + "/DevIL/CMakeLists.txt"
        tools.replace_in_file(
            main_cmakelists_path,
            "project(ImageLib)",
            "\n".join([
                "project(ImageLib)",
                "include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)",
                "conan_basic_setup()",
            ])
        )
        # Disable all projects except the main one (they have problems with shared option)
        tools.replace_in_file(main_cmakelists_path, "add_subdirectory(src-ILU)" , "# add_subdirectory(src-ILU)" )
        tools.replace_in_file(main_cmakelists_path, "add_subdirectory(src-ILUT)", "# add_subdirectory(src-ILUT)")

    def build(self):
        cmake = CMake(self)
        cmake.definitions["BUILD_SHARED_LIBS"] = self.options.shared
        cpp_flags = ''
        if self.settings.compiler == 'gcc':
            # Ignore dirty things like converting int to bool on GCC
            cpp_flags = '-fpermissive'
        cmake.definitions["CMAKE_CXX_FLAGS"] = cpp_flags
        cmake.configure(source_folder = self.source_subfolder + "/DevIL")
        cmake.build()

    def package(self):
        self.copy("il.h", dst="include/IL", src = self.source_subfolder + '/DevIL/include/IL')
        self.copy("*DevIL.lib", dst="lib", keep_path=False)
        self.copy("*libIL.a", dst="lib", keep_path=False)
        self.copy("*.dll", dst="bin", keep_path=False)
        self.copy("*.so", dst="lib", keep_path=False)
        self.copy("*.dylib", dst="lib", keep_path=False)
        self.copy("*LICENSE", dst="licenses", keep_path=False)

    def package_info(self):
        self.cpp_info.libs = ["DevIL"]

