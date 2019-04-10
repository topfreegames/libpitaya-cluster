from conans import ConanFile, CMake
import os

class TestConan(ConanFile):
    settings = 'os', 'compiler', 'build_type', 'arch'
    generators = 'cmake'

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def imports(self):
        self.copy('*.dll', dst='', src='lib', keep_path=False)
        self.copy('*.dylib', dst='', src='lib', keep_path=False)
        self.copy('*.so', dst='', src='lib', keep_path=False)

    def test(self):
        self.run('.%sexample' % os.sep)
