from conans import ConanFile, CMake, tools


class FetchppConan(ConanFile):
    name = "fetchpp"
    version = "0.14.0"
    license = "Apache 2.0"
    author = "alexandre.bossard@tanker.io"
    description = "the simplest http client"
    topics = ("http", "beast", "fetch", "client")
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "warn_as_error": [True, False],
        "coverage": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "warn_as_error": False,
        "coverage": False,
    }
    generators = "cmake"
    exports_sources = "CMakeLists.txt", "libs/*"
    cmake = None

    @property
    def should_build_tests(self):
        return self.develop and not tools.cross_building(self.settings)

    def requirements(self):
        self.requires("libressl/3.2.5")
        self.requires("boost/1.77.0")
        self.requires("nlohmann_json/3.10.2")
        self.requires("skyr-url/1.13.0-r4")

    def build_requirements(self):
        self.build_requires("catch2/2.13.6")
        self.build_requires("fmt/7.1.3")

    def init_cmake(self):
        if self.cmake:
            return
        self.cmake = CMake(self)
        self.cmake.definitions["WARN_AS_ERROR"] = self.options.warn_as_error
        self.cmake.definitions["CMAKE_POSITION_INDEPENDENT_CODE"] = self.options.fPIC
        self.cmake.definitions["BUILD_TESTING"] = self.should_build_tests
        self.cmake.definitions["WITH_COVERAGE"] = self.options.coverage
        if self.settings.os == "Windows":
            # required to build skyr-url on Windows
            self.cmake.definitions["CONAN_CMAKE_CXX_STANDARD"] = "20"

    def build(self):
        self.init_cmake()
        if self.should_configure:
            self.cmake.configure()
        if self.should_build:
            self.cmake.build()
        if self.should_build_tests and self.should_test:
            self.cmake.test()

    def package(self):
        self.init_cmake()
        self.cmake.install()

    def package_id(self):
        del self.info.options.warn_as_error

    def package_info(self):
        self.cpp_info.defines.append("BOOST_BEAST_SEPARATE_COMPILATION")
        self.cpp_info.libs = ["fetchpp"]
