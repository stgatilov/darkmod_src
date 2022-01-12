#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import shutil
import platform
from conans import ConanFile, AutoToolsBuildEnvironment, tools


class LibjpegConan(ConanFile):
    name = "libjpeg"
    version = "9c"
    description = "Libjpeg is a widely used C library for reading and writing JPEG image files."
    url = "http://github.com/bincrafters/conan-libjpeg"
    author = "Bincrafters <bincrafters@gmail.com>"
    license = "http://ijg.org/files/README"
    homepage = "http://ijg.org"
    exports = ["LICENSE.md"]
    exports_sources = ["Win32.Mak"]
    settings = "os", "arch", "compiler", "build_type"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}
    _source_subfolder = "source_subfolder"

    # thedarkmod: allow access to internal stuff (JPEG_INTERNALS)
    options["expose_internals"] = [True, False]
    default_options["expose_internals"] = True

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def configure(self):
        del self.settings.compiler.libcxx

    def source(self):
        tools.get("http://ijg.org/files/jpegsrc.v%s.tar.gz" % self.version)
        os.rename("jpeg-" + self.version, self._source_subfolder)

    def _build_nmake(self):
        if self.settings.compiler == 'Visual Studio':
            shutil.copy('Win32.Mak', os.path.join(self._source_subfolder, 'Win32.Mak'))
        with tools.chdir(self._source_subfolder):
            shutil.copy('jconfig.vc', 'jconfig.h')
            vcvars_command = tools.vcvars_command(self.settings)
            params = "nodebug=1" if self.settings.build_type == 'Release' else ""
            # set flags directly in makefile.vc
            # cflags are critical for the library. ldflags and ldlibs are only for binaries
            if self.settings.compiler.runtime in ["MD", "MDd"]:
                tools.replace_in_file('makefile.vc', '(cvars)', '(cvarsdll)')
                tools.replace_in_file('makefile.vc', '(conlibs)', '(conlibsdll)')
            else:
                tools.replace_in_file('makefile.vc', '(cvars)', '(cvarsmt)')
                tools.replace_in_file('makefile.vc', '(conlibs)', '(conlibsmt)')
            self.run('%s && nmake -f makefile.vc %s libjpeg.lib' % (vcvars_command, params))

    def _build_configure(self):
# stgatilov: platform module seems broken for me =(
#        env_build = AutoToolsBuildEnvironment(self, win_bash=self.settings.os == 'Windows' and
#                                              platform.system() == 'Windows')
        env_build = AutoToolsBuildEnvironment(self, win_bash=self.settings.os == 'Windows')
        env_build.fpic = True
        config_args = []
        if self.options.shared:
            config_args.extend(["--enable-shared=yes", "--enable-static=no"])
        else:
            config_args.extend(["--enable-shared=no", "--enable-static=yes"])
        prefix = os.path.abspath(self.package_folder)
        if self.settings.os == 'Windows':
            prefix = tools.unix_path(prefix)
        config_args.append("--prefix=%s" % prefix)

        env_build.configure(configure_dir=self._source_subfolder, args=config_args)
        env_build.make()
        env_build.install()

    def build(self):
        if self.settings.compiler == "Visual Studio":
            self._build_nmake()
        else:
            self._build_configure()

    def package(self):
        self.copy("README", src=self._source_subfolder, dst="licenses", ignore_case=True, keep_path=False)
        if self.settings.compiler == "Visual Studio":
            headers_list = ['jpeglib.h', 'jerror.h', 'jconfig.h', 'jmorecfg.h']
            # thedarkmod: add more headers when using JPEG_INTERNALS
            if self.options.expose_internals:
                headers_list.append('jpegint.h')
            for filename in headers_list:
                self.copy(pattern=filename, dst="include", src=self._source_subfolder, keep_path=False)
            self.copy(pattern="*.lib", dst="lib", src=self._source_subfolder, keep_path=False)
        shutil.rmtree(os.path.join(self.package_folder, 'share'), ignore_errors=True)
        # can safely drop bin/ because there are no shared builds
        shutil.rmtree(os.path.join(self.package_folder, 'bin'), ignore_errors=True)

    def package_info(self):
        if self.settings.compiler == "Visual Studio":
            self.cpp_info.libs = ['libjpeg']
        else:
            self.cpp_info.libs = ['jpeg']
