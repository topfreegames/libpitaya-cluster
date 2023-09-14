from os.path import join, dirname
from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMakeDeps, CMake, cmake_layout
from conan.errors import ConanInvalidConfiguration

def get_version():
    with open(join(dirname(__file__), 'version.txt'), 'r') as f:
        content = f.read()
        return content.strip()


class PitayaCpp(ConanFile):
    name = 'pitaya_cpp'
    version = get_version()
    url = 'https://github.com/topfreegames.com/libpitaya-cluster'
    description = 'C++ library that allows the creation of Pitaya servers.'
    settings = 'os', 'compiler', 'build_type', 'arch'
    options = {
        'shared': [True, False],
        'macosx_bundle': [False, True],
    }
    default_options = {
        'shared': False,
        'macosx_bundle': False,
        'cpprestsdk/*:with_websockets': False,
        'openssl/*:shared': False,
        'openssl/*:build_type': "Debug",
        'protobuf/*:debug_suffix': False,
    }
    build_requires = (
        'gtest/1.10.0'
    )
    exports = 'version.txt'

    def requirements(self):
        self.requires("zlib/1.2.13")
        self.requires("protobuf/3.21.9", force=True)
        self.requires("boost/1.83.0")
        self.requires("openssl/1.1.1v", force=True)
        self.requires("cpprestsdk/2.10.18")
        self.requires("grpc/1.50.1")
#        self.requires("spdlog/1.12.0")
        self.test_requires("gtest/1.10.0")

    def build_requirements(self):
        self.tool_requires("grpc/1.50.1")
        
    def layout(self):
        cmake_layout(self, build_folder='_builds')

    def generate(self):
        tc = CMakeToolchain(self)
        tc.presets_prefix = "npitaya"
        if self.settings.os == 'Macos':
          tc.variables['BUILD_MACOSX_BUNDLE'] = self.options.macosx_bundle
        tc.generate()

        deps = CMakeDeps(self)
        deps.generate()

    def source(self):
        self.run('git clone --branch {} --recursive https://github.com/topfreegames/libpitaya-cluster'.format(
            self.version
        ))

    def configure(self):
        if self.settings.os != 'Linux' and self.settings.os != 'Macos' and self.settings.os != 'Windows':
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
