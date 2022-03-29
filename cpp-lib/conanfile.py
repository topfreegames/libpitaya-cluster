from os.path import join, dirname
from conans import ConanFile, CMake
from conans.errors import ConanInvalidConfiguration


def get_version():
    with open(join(dirname(__file__), 'version.txt'), 'r') as f:
        content = f.read()
        return content.strip()


class PitayaCpp(ConanFile):
    name = 'pitaya_cpp'
    version = get_version()
    url = 'https://github.com/topfreegames.com/libpitaya-cluster'
    description = 'C++ library that allows the creation of Pitaya servers.'
    settings = 'cppstd', 'os', 'compiler', 'build_type', 'arch'
    options = {
        'macosx_bundle': [False, True],
    }
    default_options = {
        'macosx_bundle': False,
    }
    requires = (
        'zlib/1.2.11',
        'openssl/3.0.2',
        'boost/1.78.0',
        'protobuf/3.19.2',
    )
    build_requires = (
        'gtest/1.8.1'
    )
    generators = 'cmake_paths', 'cmake'
    exports = 'version.txt'

    def source(self):
        self.run('git clone --branch {} --recursive https://github.com/topfreegames/libpitaya-cluster'.format(
            self.version
        ))

    def configure(self):
        if self.settings.os != 'Linux' and self.settings.os != 'Macos':
            raise ConanInvalidConfiguration('%s is not supported' % self.settings.os)

    def config_options(self):
        if self.settings.os != 'Macos':
            del self.options.macosx_bundle

    def build(self):
        cmake = CMake(self)
        cmake.definitions['BUILD_TESTING'] = 'OFF'
        cmake.definitions['BUILD_SHARED_LIBS'] = 'OFF'
        cmake.definitions['CMAKE_INSTALL_PREFIX'] = 'cmake_install'
        if self.should_configure:
            cmake.configure(source_folder='libpitaya-cluster')
        if self.should_build:
            cmake.build()
        if self.should_install:
            cmake.install()

    def package(self):
        self.copy('*.h', dst='include', src='cmake_install/include')
        self.copy('*.dylib', dst='lib', src='cmake_install/lib')
        self.copy('*.so', dst='lib', src='cmake_install/lib')
        self.copy('*.bundle', dst='lib', src='cmake_install/lib')

    def package_info(self):
        self.cpp_info.libs = ['pitaya_cpp']
        self.cpp_info.libdirs = ['lib/PitayaCpp']
        self.cpp_info.cxxflags = ['-std=c++17']
