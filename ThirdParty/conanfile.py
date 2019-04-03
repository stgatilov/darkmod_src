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
    requires = (
        "zlib/1.2.11@conan/stable",
        "pugixml/1.9@bincrafters/stable",
        "libjpeg/9c@thedarkmod/local",
        "libpng/1.6.34@bincrafters/stable",
        "devil/1.7.8@thedarkmod/local",
        "openal/1.19.0@bincrafters/stable",
        "libcurl/7.61.1@thedarkmod/local",
        "ffmpeg/4.0.2@thedarkmod/local",
        "vorbis/1.3.6@bincrafters/stable",
        "tinyformat/2.1.0@thedarkmod/local",
        "doctest/2.2.0@bincrafters/stable",
    )
    default_options = {"zlib:minizip": True}

    def imports(self):
        platform = get_build_name(self.settings, False)
        for req in self.info.full_requires:
            name = req[0].name
            print(os.path.abspath("artefacts/%s/lib/%s" % (name, platform)))
            # note: we assume recipes are sane, and the set of headers does not depend on compiler/arch
            self.copy("*.h"  , root_package=name, src="include" , dst="artefacts/%s/include" % name)
            self.copy("*.hpp", root_package=name, src="include" , dst="artefacts/%s/include" % name)
            self.copy("*"    , root_package=name, src="licenses", dst="artefacts/%s/licenses" % name)
            # compiled binaries are put under compiler/arch subdirectory
            self.copy("*.lib", root_package=name, src="lib"     , dst="artefacts/%s/lib/%s" % (name, platform))
            self.copy("*.a"  , root_package=name, src="lib"     , dst="artefacts/%s/lib/%s" % (name, platform))
