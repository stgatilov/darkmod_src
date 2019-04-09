#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import re
import shutil
from conans import ConanFile, AutoToolsBuildEnvironment, RunEnvironment, CMake, tools


class LibcurlConan(ConanFile):
    name = "libcurl"
    version = "7.61.1"
    description = "command line tool and library for transferring data with URLs"
    url = "http://github.com/bincrafters/conan-libcurl"
    homepage = "http://curl.haxx.se"
    author = "Bincrafters <bincrafters@gmail.com>"
    license = "MIT"
    exports = ["LICENSE.md"]
    exports_sources = ["lib_Makefile_add.am", "CMakeLists.txt"]
    generators = "cmake"
    source_subfolder = "source_subfolder"
    settings = "os", "arch", "compiler", "build_type"
    options = {"shared": [True, False],
               "fPIC": [True, False],
               "with_openssl": [True, False],
               "with_winssl": [True, False],
               "disable_threads": [True, False],
               "with_ldap": [True, False],
               "custom_cacert": [True, False],
               "darwin_ssl": [True, False],
               "with_libssh2": [True, False],
               "with_libidn": [True, False],
               "with_librtmp": [True, False],
               "with_libmetalink": [True, False],
               "with_libpsl": [True, False],
               "with_largemaxwritesize": [True, False],
               "with_nghttp2": [True, False],
               "with_brotli": [True, False]}
    default_options = ("shared=False",
                       "fPIC=True",
                       "with_openssl=True",
                       "with_winssl=False",
                       "disable_threads=False",
                       "with_ldap=False",
                       "custom_cacert=False",
                       "darwin_ssl=True",
                       "with_libssh2=False",
                       "with_libidn=False",
                       "with_librtmp=False",
                       "with_libmetalink=False",
                       "with_libpsl=False",
                       "with_largemaxwritesize=False",
                       "with_nghttp2=False",
                       "with_brotli=False")

    @property
    def is_mingw(self):
        return self.settings.os == "Windows" and self.settings.compiler != "Visual Studio"

    @property
    def version_components(self):
        return [int(x) for x in self.version.split('.')]

    @property
    def use_brotli(self):
        return self.version_components[0] == 7 and self.version_components[1] >= 60

    def imports(self):
        # Copy shared libraries for dependencies to fix DYLD_LIBRARY_PATH problems
        #
        # Configure script creates conftest that cannot execute without shared openssl binaries.
        # Ways to solve the problem:
        # 1. set *LD_LIBRARY_PATH (works with Linux with RunEnvironment
        #     but does not work on OS X 10.11 with SIP)
        # 2. copying dylib's to the build directory (fortunately works on OS X)

        if self.settings.os == "Macos":
            self.copy("*.dylib*", dst=self.source_subfolder, keep_path=False)

    def configure(self):
        del self.settings.compiler.libcxx

        # be careful with those flags:
        # - with_openssl AND darwin_ssl uses darwin_ssl (to maintain recipe compatibilty)
        # - with_openssl AND NOT darwin_ssl uses openssl
        # - with_openssl AND with_winssl raises to error
        # - with_openssl AND NOT with_winssl uses openssl
        # Moreover darwin_ssl is set by default and with_winssl is not

        if self.options.with_openssl:
            # enforce shared linking due to openssl dependency
            if self.settings.os != "Macos" or not self.options.darwin_ssl:
                self.options["OpenSSL"].shared = self.options.shared
        if self.options.with_libssh2:
            if self.settings.compiler != "Visual Studio":
                self.options["libssh2"].shared = self.options.shared

    def config_options(self):

        if self.settings.os != "Macos":
            try:
                self.options.remove("darwin_ssl")
            except:
                pass
        if self.settings.os != "Windows":
            try:
                self.options.remove("with_winssl")
            except:
                pass

        if self.settings.os == "Windows" and self.options.with_winssl and self.options.with_openssl:
            raise Exception('Specify only with_winssl or with_openssl')

        # libpsl is supported for libcurl >= 7.46.0
        use_libpsl = self.version_components[0] == 7 and self.version_components[1] >= 46
        if not use_libpsl:
            self.options.remove('with_libpsl')

        if not self.use_brotli:
            self.options.remove('with_brotli')

        if self.settings.os == "Windows":
            self.options.remove("fPIC")

    def requirements(self):
        if self.options.with_openssl:
            # libcurl before 7.56.0 supported openssl only experimentally on Windows (cmake). warn about it
            if self.settings.os == "Windows" and self.version_components[1] < 56:
                self.output.warn("OpenSSL is supported experimentally, use at your own risk")

            if self.settings.os == "Macos" and self.options.darwin_ssl:
                pass
            elif self.settings.os == "Windows" and self.options.with_winssl:
                pass
            else:
                self.requires.add("OpenSSL/1.0.2n@conan/stable")
        if self.options.with_libssh2:
            if self.settings.compiler != "Visual Studio":
                self.requires.add("libssh2/1.8.0@bincrafters/stable")

        self.requires.add("zlib/1.2.11@conan/stable")

    def source(self):
        tools.get("https://curl.haxx.se/download/curl-%s.tar.gz" % self.version)
        os.rename("curl-%s" % self.version, self.source_subfolder)
        tools.download("https://curl.haxx.se/ca/cacert.pem", "cacert.pem", verify=False)
        os.rename(os.path.join(self.source_subfolder, "CMakeLists.txt"),
                  os.path.join(self.source_subfolder, "CMakeLists_original.txt"))
        shutil.copy("CMakeLists.txt",
                    os.path.join(self.source_subfolder, "CMakeLists.txt"))

    def build(self):
        self.patch_misc_files()
        if self.settings.compiler != "Visual Studio":
            self.build_with_autotools()
        else:
            self.build_with_cmake()

    def package(self):
        # Everything is already installed by make install
        self.copy(pattern="COPYING*", dst="licenses", src=self.source_subfolder, ignore_case=True, keep_path=False)

        # Copy the certs to be used by client
        self.copy("cacert.pem", keep_path=False)

        if self.settings.os == "Windows" and self.settings.compiler != "Visual Studio":
            # Handle only mingw libs
            self.copy(pattern="*.dll", dst="bin", keep_path=False)
            self.copy(pattern="*dll.a", dst="lib", keep_path=False)
            self.copy(pattern="*.def", dst="lib", keep_path=False)
            self.copy(pattern="*.lib", dst="lib", keep_path=False)

        # no need to distribute docs/man pages
        shutil.rmtree(os.path.join(self.package_folder, 'share', 'man'), ignore_errors=True)
        # no need for bin tools
        for binname in ['curl', 'curl.exe']:
            if os.path.isfile(os.path.join(self.package_folder, 'bin', binname)):
                os.remove(os.path.join(self.package_folder, 'bin', binname))

    def package_info(self):
        if self.settings.compiler != "Visual Studio":
            self.cpp_info.libs = ['curl']
            if self.settings.os == "Linux":
                self.cpp_info.libs.extend(["rt", "pthread"])
                if self.options.with_libssh2:
                    self.cpp_info.libs.extend(["ssh2"])
                if self.options.with_libidn:
                    self.cpp_info.libs.extend(["idn"])
                if self.options.with_librtmp:
                    self.cpp_info.libs.extend(["rtmp"])
                if self.use_brotli and self.options.with_brotli:
                    self.cpp_info.libs.extend(["brotlidec"])
            if self.settings.os == "Macos":
                if self.options.with_ldap:
                    self.cpp_info.libs.extend(["ldap"])
                if self.options.darwin_ssl:
                    self.cpp_info.exelinkflags.append("-framework Cocoa")
                    self.cpp_info.exelinkflags.append("-framework Security")
                    self.cpp_info.sharedlinkflags = self.cpp_info.exelinkflags
        else:
            self.cpp_info.libs = ['libcurl_imp'] if self.options.shared else ['libcurl']
            self.cpp_info.libs.append('Ws2_32')
            if self.options.with_ldap:
                self.cpp_info.libs.append("wldap32")

        if not self.options.shared:
            self.cpp_info.defines.append("CURL_STATICLIB=1")

    def patch_misc_files(self):
        if self.options.with_largemaxwritesize:
            tools.replace_in_file(os.path.join(self.source_subfolder, 'include', 'curl', 'curl.h'),
                                  "define CURL_MAX_WRITE_SIZE 16384",
                                  "define CURL_MAX_WRITE_SIZE 10485760")

        # BUG 2121, introduced in 7.55.0, fixed in 7.61.0
        # workaround for DEBUG_POSTFIX
        if self.version_components[0] == 7 and self.version_components[1] >= 55 and self.version_components[1] < 61:
            tools.replace_in_file(os.path.join(self.source_subfolder, 'lib', 'CMakeLists.txt'),
                                  '  DEBUG_POSTFIX "-d"',
                                  '  DEBUG_POSTFIX ""')

        # BUG 1788, fixed in 7.56.0
        # connectx() is for >=10.11 so it raised unguarded-availability error.
        # Patch the file to avoid calling it instead of blinding ignoring error
        if self.settings.os == "Macos":
            if self.version_components[0] == 7 and self.version_components[1] < 56:
                tools.replace_in_file(os.path.join(self.source_subfolder, 'lib', 'connect.c'),
                                      '#if defined(CONNECT_DATA_IDEMPOTENT)',
                                      '#if 0')

        # https://github.com/curl/curl/issues/2835
        if self.settings.compiler == 'apple-clang' and self.settings.compiler.version == '9.1':
            if self.options.darwin_ssl:
                if self.version_components[0] == 7 and self.version_components[1] >= 56 and self.version_components[2] >= 1:
                    tools.replace_in_file(os.path.join(self.source_subfolder, 'lib', 'vtls', 'darwinssl.c'),
                                          '#define CURL_BUILD_MAC_10_13 MAC_OS_X_VERSION_MAX_ALLOWED >= 101300',
                                          '#define CURL_BUILD_MAC_10_13 0')

    def get_configure_command_args(self):
        params = []
        use_idn2 = self.version_components[0] == 7 and self.version_components[1] >= 53
        if use_idn2:
            params.append("--without-libidn2" if not self.options.with_libidn else "--with-libidn2")
        else:
            params.append("--without-libidn" if not self.options.with_libidn else "--with-libidn")
        params.append("--without-librtmp" if not self.options.with_librtmp else "--with-librtmp")
        params.append("--without-libmetalink" if not self.options.with_libmetalink else "--with-libmetalink")
        params.append("--without-libpsl" if not self.options.with_libpsl else "--with-libpsl")
        params.append("--without-nghttp2" if not self.options.with_nghttp2 else "--with-nghttp2")
        if self.use_brotli:
            params.append("--without-brotli" if not self.options.with_brotli else "--with-brotli")

        if self.settings.os == "Macos" and self.options.darwin_ssl:
            params.append("--with-darwinssl")
            params.append("--without-ssl")
        elif self.settings.os == "Windows" and self.options.with_winssl:
            params.append("--with-winssl")
            params.append("--without-ssl")
        elif self.options.with_openssl:
            openssl_path = self.deps_cpp_info["OpenSSL"].rootpath.replace('\\', '/')
            params.append("--with-ssl=%s" % openssl_path)
        else:
            params.append("--without-ssl")

        if self.options.with_libssh2:
            params.append("--with-libssh2=%s" % self.deps_cpp_info["libssh2"].lib_paths[0].replace('\\', '/'))
        else:
            params.append("--without-libssh2")

        params.append("--with-zlib=%s" % self.deps_cpp_info["zlib"].lib_paths[0].replace('\\', '/'))

        if not self.options.shared:
            params.append("--disable-shared")
            params.append("--enable-static")
        else:
            params.append("--enable-shared")
            params.append("--disable-static")

        if self.options.disable_threads:
            params.append("--disable-thread")

        if not self.options.with_ldap:
            params.append("--disable-ldap")

        if self.options.custom_cacert:
            params.append('--with-ca-bundle=cacert.pem')

        # Cross building flags
        if tools.cross_building(self.settings):
            if self.settings.os == "Linux" and "arm" in self.settings.arch:
                params.append('--host=%s' % self.get_linux_arm_host())

        return params

    def get_linux_arm_host(self):
        arch = None
        if self.settings.os == 'Linux':
            arch = 'arm-linux-gnu'
            # aarch64 could be added by user
            if 'aarch64' in self.settings.arch:
                arch = 'aarch64-linux-gnu'
            elif 'arm' in self.settings.arch and 'hf' in self.settings.arch:
                arch = 'arm-linux-gnueabihf'
            elif 'arm' in self.settings.arch and self.arm_version(str(self.settings.arch)) > 4:
                arch = 'arm-linux-gnueabi'
        return arch

    def arm_version(self, arch):
        version = None
        match = re.match(r"arm\w*(\d)", arch)
        if match:
            version = int(match.group(1))
        return version

    def patch_mingw_files(self):
        if not self.is_mingw:
            return
        # patch autotools files
        with tools.chdir(self.source_subfolder):
            # for mingw builds - do not compile curl tool, just library
            # linking errors are much harder to fix than to exclude curl tool
            if self.version_components[0] == 7 and self.version_components[1] >= 55:
                tools.replace_in_file("Makefile.am",
                                      'SUBDIRS = lib src',
                                      'SUBDIRS = lib')
            else:
                tools.replace_in_file("Makefile.am",
                                      'SUBDIRS = lib src include',
                                      'SUBDIRS = lib include')

            tools.replace_in_file("Makefile.am",
                                  'include src/Makefile.inc',
                                  '')

            # patch for zlib naming in mingw
            tools.replace_in_file("configure.ac",
                                  '-lz ',
                                  '-lzlib ')

            if self.options.shared:
                # patch for shared mingw build
                tools.replace_in_file(os.path.join('lib', 'Makefile.am'),
                                      'noinst_LTLIBRARIES = libcurlu.la',
                                      '')
                tools.replace_in_file(os.path.join('lib', 'Makefile.am'),
                                      'noinst_LTLIBRARIES =',
                                      '')
                tools.replace_in_file(os.path.join('lib', 'Makefile.am'),
                                      'lib_LTLIBRARIES = libcurl.la',
                                      'noinst_LTLIBRARIES = libcurl.la')
                # add directives to build dll
                added_content = tools.load(os.path.join(self.source_folder, 'lib_Makefile_add.am'))
                tools.save(os.path.join('lib', 'Makefile.am'), added_content, append=True)

    def build_with_autotools(self):
        autotools = AutoToolsBuildEnvironment(self, win_bash=self.is_mingw)

        if self.settings.os != "Windows":
            autotools.fpic = self.options.fPIC

        autotools_vars = autotools.vars
        # tweaks for mingw
        if self.is_mingw:
            # patch autotools files
            self.patch_mingw_files()

            autotools.defines.append('_AMD64_')
            autotools_vars['RCFLAGS'] = '-O COFF'
            if self.settings.arch == "x86":
                autotools_vars['RCFLAGS'] += ' --target=pe-i386'
            else:
                autotools_vars['RCFLAGS'] += ' --target=pe-x86-64'

            del autotools_vars['LIBS']

        self.output.info("Autotools env vars: " + repr(autotools_vars))

        env_run = RunEnvironment(self)
        # run configure with *LD_LIBRARY_PATH env vars
        # it allows to pick up shared openssl
        self.output.info("Run vars: " + repr(env_run.vars))
        with tools.environment_append(env_run.vars):
            with tools.chdir(self.source_subfolder):
                # autoreconf
                self.run('./buildconf', win_bash=self.is_mingw)

                # fix generated autotools files
                tools.replace_in_file("configure", "-install_name \\$rpath/", "-install_name ")

                # BUG 1420, fixed in 7.54.1
                if self.version_components[0] == 7 and self.version_components[1] < 55:
                    tools.replace_in_file("configure",
                                          'LDFLAGS="`$PKGCONFIG --libs-only-L zlib` $LDFLAGS"',
                                          'LDFLAGS="$LDFLAGS `$PKGCONFIG --libs-only-L zlib`"')

                self.run("chmod +x configure")
                configure_args = self.get_configure_command_args()
                autotools.configure(vars=autotools_vars, args=configure_args)
                autotools.make(vars=autotools_vars)
                autotools.install(vars=autotools_vars)

    def build_with_cmake(self):
        # patch cmake files
        with tools.chdir(self.source_subfolder):
            tools.replace_in_file("CMakeLists_original.txt",
                                  "include(CurlSymbolHiding)",
                                  "")

        cmake = CMake(self)
        cmake.definitions['BUILD_TESTING'] = False
        cmake.definitions['BUILD_CURL_EXE'] = False
        cmake.definitions['CURL_DISABLE_LDAP'] = not self.options.with_ldap
        cmake.definitions['BUILD_SHARED_LIBS'] = self.options.shared
        cmake.definitions['CURL_STATICLIB'] = not self.options.shared
        cmake.definitions['CMAKE_DEBUG_POSTFIX'] = ''
        cmake.definitions['CMAKE_USE_LIBSSH2'] = self.options.with_libssh2

        # all these options are exclusive. set just one of them
        # mac builds do not use cmake so don't even bother about darwin_ssl
        cmake.definitions['CMAKE_USE_WINSSL'] = 'with_winssl' in self.options and self.options.with_winssl
        cmake.definitions['CMAKE_USE_OPENSSL'] = 'with_openssl' in self.options and self.options.with_openssl

        if self.settings.compiler != 'Visual Studio':
            cmake.definitions['CMAKE_POSITION_INDEPENDENT_CODE'] = self.options.fPIC
        cmake.configure(source_dir=self.source_subfolder)
        cmake.build()
        cmake.install()
