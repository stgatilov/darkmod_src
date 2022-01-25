from conans import ConanFile
import os


def get_platform_name(settings, shared=False):
    os = {'Windows': 'win', 'Linux': 'lnx'}[str(settings.os)]
    bitness = {'x86': '32', 'x86_64': '64'}[str(settings.arch)]
    dynamic = 'd' if shared else 's'
    compiler = {'Visual Studio': 'vc', 'gcc': 'gcc'}[str(settings.compiler)]
    # GCC 5-10 are binary compatible, MSVC 2015-2019 are compatible too
    # see also: https://forums.thedarkmod.com/index.php?/topic/20940-unable-to-link-openal-during-compilation/
    ### if compiler in ['vc', 'gcc']:
    ###     compiler += str(settings.compiler.version)
    buildtype = {'Release': 'rel', 'Debug': 'dbg', 'RelWithDebInfo': 'rwd'}[str(settings.build_type)]
    stdlib = '?'
    if compiler.startswith('vc'):
        stdlib = str(settings.compiler.runtime).lower()
    elif compiler.startswith('gcc'):
        stdlib = {'libstdc++': 'stdcpp'}[str(settings.compiler.libcxx)]
    return '%s%s_%s_%s_%s_%s' % (os, bitness, dynamic, compiler, buildtype, stdlib)


class TdmDepends(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "with_headeronly": [True, False],
        "with_releaseonly": [True, False],
        "with_perbuild": [True, False],
        "platform_name": "ANY",
    }
    default_options = {
        "with_headeronly": True,
        "with_releaseonly": True,
        "with_perbuild": True,
        "platform_name": None,
        # build minizip too (it is part of zlib package)
        "zlib:minizip": True,
        # enable SSE2 in hash library
        "BLAKE2:SSE": "SSE2",
        # avoid fragile selection of system packages in glfw
        "glfw:no_opengl": True,
        "glfw:no_xorg": True,
    }

    # these deps are header-only, hence they don't need per-configuration builds
    requires_headeronly = (
        "tinyformat/2.1.0@thedarkmod/local",
        "doctest/2.2.3@thedarkmod/local",
        "tracy/0.7.8@thedarkmod/local",
    )
    # these deps can be built only once in release mode, and then used both in release and debug builds
    # this is possible only for pure C libraries built with /MT, and needs /nodefaultlib:LIBCMT.lib flag in debug build
    requires_releaseonly = (
        "zlib/1.2.11@#514b772abf9c36ad9be48b84cfc6fdc2",
        "libcurl/7.61.1@thedarkmod/local",
        "libjpeg/9c@thedarkmod/local",
        "libpng/1.6.34@bincrafters/stable",
        "ffmpeg/4.0.2@thedarkmod/local",
        "vorbis/1.3.6@bincrafters/stable",
        "fltk/1.3.5@thedarkmod/local",
        "BLAKE2/master@thedarkmod/local",
        "glfw/3.3.4@thedarkmod/local",
    )
    # these deps must be built separately for each configuration (both debug and release)
    # this is required for C++ libs because iterator debugging and runtime differences are not allowed by MSVC
    requires_perbuild = (
        "openal/1.21.1@thedarkmod/local",
        "pugixml/1.9@bincrafters/stable",
    )

    def requirements(self):
        if self.options.with_headeronly:
            for dep in self.requires_headeronly:
                self.requires.add(dep)
        if self.options.with_releaseonly:
            for dep in self.requires_releaseonly:
                self.requires.add(dep)
        if self.options.with_perbuild:
            for dep in self.requires_perbuild:
                self.requires.add(dep)

    def imports(self):
        if self.options.platform_name == "None":
            self.options.platform_name = get_platform_name(self.settings, False)
        platform = self.options.platform_name
        for req in self.info.full_requires:
            name = req[0].name
            print(os.path.abspath("artefacts/%s/lib/%s" % (name, platform)))
            # note: we assume recipes are sane, and the set of headers does not depend on compiler/arch
            self.copy("*.h"  , root_package=name, src="include" , dst="artefacts/%s/include" % name)
            self.copy("*.H"  , root_package=name, src="include" , dst="artefacts/%s/include" % name)    # FLTK =(
            self.copy("*.hpp", root_package=name, src="include" , dst="artefacts/%s/include" % name)
            self.copy("*"    , root_package=name, src="licenses", dst="artefacts/%s/licenses" % name)
            # source code files to be embedded into build (used by Tracy)
            self.copy("*.cpp", root_package=name, src="src" , dst="artefacts/%s/src" % name)
            self.copy("*.c"  , root_package=name, src="src" , dst="artefacts/%s/src" % name)
            # compiled binaries are put under subdirectory named by build settings
            self.copy("*.lib", root_package=name, src="lib"     , dst="artefacts/%s/lib/%s" % (name, platform))
            self.copy("*.a"  , root_package=name, src="lib"     , dst="artefacts/%s/lib/%s" % (name, platform))
            # while we don't use dynamic libraries, some packages provide useful executables (e.g. FLTK gives fluid.exe)
            self.copy("*.dll", root_package=name, src="bin"     , dst="artefacts/%s/bin/%s" % (name, platform))
            self.copy("*.so" , root_package=name, src="bin"     , dst="artefacts/%s/bin/%s" % (name, platform))
            self.copy("*.exe", root_package=name, src="bin"     , dst="artefacts/%s/bin/%s" % (name, platform))
            self.copy("*.bin", root_package=name, src="bin"     , dst="artefacts/%s/bin/%s" % (name, platform))
