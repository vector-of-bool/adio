cmake_minimum_required(VERSION 3.0.0)
project(adio VERSION 0.0.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

include(CTest)
enable_testing()

find_package(Pew QUIET)
include(CMakePackageConfigHelpers)


if(Pew_FOUND)
    find_package(boost QUIET)
    find_package(asio QUIET)
    find_package(sqlite REQUIRED)
else()
    include(cmake/ImportBoost.cmake)
    include(cmake/ImportSQLite.cmake)
    find_package(Boost QUIET)
endif()

if(NOT boost_FOUND AND NOT Boost_FOUND)
    set(have_boost NO)
    find_package(asio QUIET)
    set(default_backend asio)
else()
    set(have_boost TRUE)
    set(default_backend boost)
endif()

set(can_build TRUE)
if(NOT have_boost AND NOT asio_FOUND)
    message(SEND_ERROR "Cannot build tests/examples without either Boost or asio installed")
endif()

set(ASIO_BACKEND "${default_backend}" CACHE STRING "Choose the asio type to use (boost/vanilla)")
set_property(CACHE ASIO_BACKEND PROPERTY STRINGS boost vanilla)
message(STATUS "Targeting the \"${ASIO_BACKEND}\" asio backend")

if(ASIO_BACKEND STREQUAL vanilla)
    find_package(asio REQUIRED)
endif()

if(WIN32)
    set(ADIO_CMAKE_PREFIX cmake)
else()
    set(ADIO_CMAKE_PREFIX ${PROJECT_NAME}-${PROJECT_VERSION}/cmake)
endif()

add_subdirectory(source)
if(BUILD_TESTING)
    find_package(catch)
    if(NOT catch_FOUND)
        message(WARNING "Cannot build the tests without Catch")
    else()
        add_subdirectory(tests)
    endif()
endif()

if(Pew_FOUND)
    pew_export_project()
else()
    install(EXPORT adioTargets DESTINATION ${ADIO_CMAKE_PREFIX})
    write_basic_package_version_file(${PROJECT_NAME}ConfigVersion.cmake COMPATIBILITY SameMajorVersion)
    configure_package_config_file(${PROJECT_NAME}Config.in.cmake ${PROJECT_NAME}Config.cmake INSTALL_DESTINATION ${ADIO_CMAKE_PREFIX})
    install(
        FILES
            ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
        DESTINATION ${ADIO_CMAKE_PREFIX}
        )
endif()

find_program(DOXYGEN_EXECUTABLE doxygen DOC "Path to Doxygen, for generating documentation")
set(doxy_js ${CMAKE_CURRENT_LIST_DIR}/docs/doxy-boot.js)
set(doxy_css ${CMAKE_CURRENT_LIST_DIR}/docs/customdoxygen.css)
if(DOXYGEN_EXECUTABLE AND EXISTS "${doxy_js}")
    configure_file(Doxyfile.in Doxyfile @ONLY)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/docs)
    add_custom_target(docs
        COMMAND "${DOXYGEN_EXECUTABLE}" "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
        #COMMAND "${CMAKE_COMMAND}" -E copy "${doxy_css}" "${CMAKE_CURRENT_BINARY_DIR}/docs/html"
        COMMAND "${CMAKE_COMMAND}" -E copy "${doxy_js}" "${CMAKE_CURRENT_BINARY_DIR}/docs/html"
        COMMENT "Generating documentation"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/docs"
        USES_TERMINAL
        )
endif()

set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_OUTPUT_FILE_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/packages)
include(CPack)
