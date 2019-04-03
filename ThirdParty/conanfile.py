from conans import ConanFile
import os

def get_platform_name(settings):
    arch = None
    if settings.arch == 'x86':
        arch = 'x32'
    elif settings.arch == 'x86_64':
        arch = 'x64'
    else:
        assert False, "unknown architecture %s" % settings.arch

    if settings.compiler == 'Visual Studio':
        return 'win_vc%s_%s' % (settings.compiler.version, arch)
    elif settings.compiler == 'gcc':
        return 'lnx_gcc_%s' % arch
    else:
        assert False, "unknown compiler %s" % settings.compiler


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
    )
    default_options = {"zlib:minizip": True}

    def imports(self):
        platform = get_platform_name(self.settings)
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
