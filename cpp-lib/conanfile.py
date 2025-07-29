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
        'protobuf/*:debug_suffix': False,
        'cnats/*:shared': False,
        'cnats/*:with_streaming': False,
        'cnats/*:with_tls': True,
        'cnats/*:tls_use_openssl_1_1_api': True,
        'etcd-cpp-apiv3/*:with_nats': True,
        'etcd-cpp-apiv3/*:nats_package_name': 'cnats',
    }
    build_requires = (
        'gtest/1.10.0'
    )
    exports = 'version.txt'

    def requirements(self):
        self.requires("zlib/1.3.1")
        self.requires("protobuf/3.21.9", visible=True, force=True)
        self.requires("boost/1.83.0")
        self.requires("openssl/3.0.8", force=True)
        self.requires("grpc/1.54.3")
        self.requires("protobuf-c/1.5.0")
        self.requires("etcd-cpp-apiv3/0.15.4")
        self.requires("cnats/3.10.1")
        self.test_requires("gtest/1.10.0")

    def build_requirements(self):
        self.tool_requires("grpc/1.54.3")
        self.tool_requires("protobuf/3.21.9")

    def layout(self):
        cmake_layout(self, build_folder='_builds')

    def generate(self):
        tc = CMakeToolchain(self)
        tc.presets_prefix = "conan"
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
        self.copy('*.dll', dst='lib', src='cmake_install/lib')
        self.copy('*.lib', dst='lib', src='cmake_install/lib')

    def package_info(self):
        self.cpp_info.libs = ['pitaya_cpp']
        self.cpp_info.libdirs = ['lib/PitayaCpp']
        if self.settings.os == 'Windows':
            self.cpp_info.cxxflags = ['/std:c++17']
        else:
            self.cpp_info.cxxflags = ['-std=c++17']
