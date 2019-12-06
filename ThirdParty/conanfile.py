from conans import ConanFile
import os


def get_build_name(settings, shared=False):
    os = {'Windows': 'win', 'Linux': 'lnx'}[str(settings.os)]
    bitness = {'x86': '32', 'x86_64': '64'}[str(settings.arch)]
    dynamic = 'd' if shared else 's'
    compiler = {'Visual Studio': 'vc', 'gcc': 'gcc'}[str(settings.compiler)]
    if compiler in ['vc', 'gcc']:
        compiler += str(settings.compiler.version)
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
    }
    default_options = {
        "with_headeronly": True,
        "with_releaseonly": True,
        "with_perbuild": True,
        # build minizip too (it is part of zlib package)
        "zlib:minizip": True,
    }

    # these deps are header-only, hence they don't need per-configuration builds
    requires_headeronly = (
        "tinyformat/2.1.0@thedarkmod/local",
        "doctest/2.2.3@thedarkmod/local",
    )
    # these deps can be built only once in release mode, and then used both in release and debug builds
    # this is possible only for pure C libraries built with /MT, and needs /nodefaultlib:LIBCMT.lib flag in debug build
    requires_releaseonly = (
        "zlib/1.2.11@conan/stable",
        "libcurl/7.61.1@thedarkmod/local",
        "devil/1.7.8@thedarkmod/local",
        "libjpeg/9c@thedarkmod/local",
        "libpng/1.6.34@bincrafters/stable",
        "ffmpeg/4.0.2@thedarkmod/local",
        "openal/1.19.1@thedarkmod/local",
        "vorbis/1.3.6@bincrafters/stable",
    )
    # these deps must be built separately for each configuration (both debug and release)
    # this is required for C++ libs because iterator debugging and runtime differences are not allowed by MSVC
    requires_perbuild = (
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
        platform = get_build_name(self.settings, False)
        for req in self.info.full_requires:
            name = req[0].name
            print(os.path.abspath("artefacts/%s/lib/%s" % (name, platform)))
            # note: we assume recipes are sane, and the set of headers does not depend on compiler/arch
            self.copy("*.h"  , root_package=name, src="include" , dst="artefacts/%s/include" % name)
            self.copy("*.hpp", root_package=name, src="include" , dst="artefacts/%s/include" % name)
            self.copy("*"    , root_package=name, src="licenses", dst="artefacts/%s/licenses" % name)
            # compiled binaries are put under subdirectory named by build settings
            self.copy("*.lib", root_package=name, src="lib"     , dst="artefacts/%s/lib/%s" % (name, platform))
            self.copy("*.a"  , root_package=name, src="lib"     , dst="artefacts/%s/lib/%s" % (name, platform))
