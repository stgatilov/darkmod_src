from conans import CMake, ConanFile, tools
import os, shutil, glob

class FltkConan(ConanFile):
    name = "fltk"
    version = "1.3.5"
    description = "Fast Light Toolkit is a cross-platform C++ GUI toolkit"
    topics = ("conan", "fltk", "gui")
    homepage = "https://www.fltk.org"
    license = "LGPL-2.0-custom"
    author = "stgatilov <stgatilov@gmail.com>"
    url = "https://github.com/stgatilov/conan-fltk"
    exports = ["LICENSE"]
    exports_sources = ["CMakeLists.txt"]
    generators = "cmake"
    settings = "os", "arch", "compiler", "build_type"

    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "use_gl": [True, False],
        "use_threads": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "use_gl": True,
        "use_threads": True,
    }
    requires = [
        "zlib/1.2.11",
        "libjpeg/9c",
        "libpng/1.6.37",
    ]

    _source_subfolder = "source_dir"

    def source(self):
        source_url = "https://www.fltk.org/pub/fltk/{0}/fltk-{0}-source.tar.gz".format(self.version)
        md5 = {
            "1.3.5": "e85017defd5a03ae82e634311db87bbf",
        }.get(self.version, "")
        tools.get(source_url, md5=md5)
        extracted_dir = "fltk-{0}".format(self.version)
        os.rename(extracted_dir, self._source_subfolder)

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def _configure_cmake(self):
        cmake = CMake(self)
        cmake.definitions['OPTION_BUILD_SHARED_LIBS'] = self.options.shared
        cmake.definitions['OPTION_BUILD_EXAMPLES'] = False
        cmake.definitions['OPTION_USE_GL'] = self.options.use_gl
        cmake.definitions['OPTION_USE_THREADS'] = self.options.use_threads
        # how to use such X-specific system libs?
        cmake.definitions['OPTION_USE_XDBE'] = False
        cmake.definitions['OPTION_USE_XRENDER'] = False
        cmake.definitions['OPTION_USE_XFT'] = False
        cmake.definitions['OPTION_USE_XCURSOR'] = False
        cmake.definitions['OPTION_USE_XFIXES'] = False
        cmake.definitions['OPTION_USE_XINERAMA'] = False
        cmake.configure()
        return cmake

    def build(self):
        cmake = self._configure_cmake()
        cmake.build()

    def package(self):
        cmake = self._configure_cmake()
        cmake.install()
        self.copy(self._source_subfolder + "/COPYING", dst="licenses", keep_path=False, ignore_case=True)
        # these headers and libs come from dependencies
        deps_includes = self.package_folder + "/include/FL/images"
        if os.path.isdir(deps_includes):
            shutil.rmtree(deps_includes)
        removed_libs = ["z", "jpeg", "png"]
        for rlib in removed_libs:
            for fn in glob.glob(self.package_folder + "/*/*fltk_" + rlib + "*.*"):
                os.remove(fn)       # lib/fltk_png.lib, bin/libfltk_png_SHARED.dll
        if self.options.shared:
            for fn in glob.glob(self.package_folder + "/lib/*.lib"):
                if '_SHARED' not in fn:
                    os.remove(fn)   # static libraries

    def package_info(self):
        if self.options.shared:
            self.cpp_info.defines.append("FL_DLL")
        self.cpp_info.libs = tools.collect_libs(self)
        if self.settings.os == "Linux":
            self.cpp_info.libs.extend(['m', 'dl', 'X11', 'Xext'])
            if self.options.use_threads:
                self.cpp_info.libs.extend(['pthread'])
            if self.options.use_gl:
                self.cpp_info.libs.extend(['GL', 'GLU'])
