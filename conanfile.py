from conans import ConanFile, CMake, tools


class FetchppConan(ConanFile):
    name = "fetchpp"
    version = "0.0.1"
    license = "Apache 2.0"
    author = "alexandre.bossard@tanker.io"
    description = "the simplest http client"
    topics = ("http", "beast", "fetch", "client")
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "with_ssl": [True, False],
        "warn_as_error": [True, False]
    }
    default_options = {"shared": False, "fPIC": False, "with_ssl": True, "warn_as_error": False}
    generators = "cmake"
    exports_sources = "CMakeLists.txt", "libs/*"
    cmake = None


    @property
    def should_build_tests(self):
        return self.develop and not tools.cross_building(self.settings)


    def requirements(self):
        self.requires("Boost/1.71.0@tanker/testing")


    def build_requirements(self):
        self.build_requires("jsonformoderncpp/3.4.0@tanker/testing")
        self.build_requires("Catch2/2.10.0@catchorg/stable")
        self.build_requires("fmt/6.0.0")


    def init_cmake(self):
        if self.cmake:
            return
        self.cmake = CMake(self)
        self.cmake.definitions["WARN_AS_ERROR"] = self.options.warn_as_error
        self.cmake.definitions["CMAKE_POSITION_INDEPENDENT_CODE"] = self.options.fPIC
        self.cmake.definitions["ENABLE_TESTING"] = self.should_build_tests


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
        self.cpp_info.libs = ["fecthpp"]
