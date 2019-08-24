cmake_minimum_required(VERSION 3.15)
project(adio)

include(pmm.cmake)
pmm(CONAN
    REMOTES vob https://api.bintray.com/conan/vector-of-bool/vector-of-bool
    CMakeCM ROLLING
    )

include(${CMAKE_BINARY_DIR}/libman.cmake)
import_packages(catch2 Boost)
find_package(Catch2 REQUIRED)
include(Catch)

find_package(Threads REQUIRED)

include(tools/catch-main.cmake)

enable_testing()

export_package()

set(ADIO_BACKEND "Boost" CACHE STRING "The backend for Adio")
set_property(CACHE ADIO_BACKEND PROPERTY STRINGS Boost Asio NetworkTS)

add_subdirectory(libs)
