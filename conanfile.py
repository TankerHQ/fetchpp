from conans import ConanFile, CMake, tools


class FetchppConan(ConanFile):
    name = "fetchpp"
    version = "dev"
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
        self.requires("libressl/3.2.0")
        self.requires("boost/1.76.0")
        self.requires("nlohmann_json/3.8.0")
        self.requires("skyr-url/1.12.0")

    def build_requirements(self):
        self.build_requires("catch2/2.12.2")
        self.build_requires("fmt/7.0.2")

    def init_cmake(self):
        if self.cmake:
            return
        self.cmake = CMake(self)
        self.cmake.definitions["WARN_AS_ERROR"] = self.options.warn_as_error
        self.cmake.definitions["CMAKE_POSITION_INDEPENDENT_CODE"] = self.options.fPIC
        self.cmake.definitions["BUILD_TESTING"] = self.should_build_tests
        self.cmake.definitions["WITH_COVERAGE"] = self.options.coverage

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
        self.cpp_info.defines.append("BOOST_BEAST_USE_STD_STRING_VIEW")
        self.cpp_info.libs = ["fetchpp"]
