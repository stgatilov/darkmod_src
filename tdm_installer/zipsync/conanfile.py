from conans import ConanFile, CMake, tools

class ZipsyncConan(ConanFile):
    name = "zipsync"
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake_find_package"
    requires = [
        "zlib/1.2.11@conan/stable",
        "libcurl/7.60.0@bincrafters/stable",
        "BLAKE2/master@zipsync/local",
        "doctest/2.3.1@bincrafters/stable",
        "args/6.2.2@pavel-belikov/stable",
        "libmicrohttpd/0.9.59@zipsync/local",
    ]

    def configure(self):
        self.options["zlib"].minizip = True
        self.options["libcurl"].with_openssl = False
        if self.settings.os == "Windows":
            self.options["libcurl"].with_winssl = False
        self.options["BLAKE2"].SSE = "SSE2"

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
