include(CMakeParseArguments)

add_library(adio
    dummy.cpp
    adio/config.hpp
    adio/utils.hpp
    adio/traits.hpp
    adio/error.hpp
    adio/error.cpp
    adio/connection_fwd.hpp
    adio/connection.hpp
    adio/connection.cpp
    adio/driver.hpp
    adio/value.hpp
    adio/value.cpp
    adio/row.hpp
    )

target_link_libraries(adio PUBLIC boost::variant)

if(ASIO_BACKEND STREQUAL boost)
    target_compile_definitions(adio PUBLIC ADIO_DETAIL_USE_BOOST_ASIO_DEFAULT)
    target_link_libraries(adio PUBLIC $<BUILD_INTERFACE:boost::asio>)
elseif(ASIO_BACKEND STREQUAL vanilla)
    target_compile_definitions(adio PUBLIC ADIO_DETAIL_USE_VANILLA_ASIO_DEFAULT)
    target_link_libraries(adio PUBLIC $<BUILD_INTERFACE:asio::asio>)
else()
    message(SEND_ERROR "Unknown ASIO_BACKEND: ${ASIO_BACKEND}")
endif()

target_compile_features(adio PUBLIC cxx_alias_templates cxx_auto_type cxx_variadic_templates cxx_lambdas)
set_property(TARGET adio PROPERTY CXX_EXTENSIONS FALSE)

target_include_directories(adio PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>)

if(NOT MSVC)
    target_link_libraries(adio PUBLIC -pthread)
endif()

add_library(adio::adio ALIAS adio)

function(adio_backend name)
    cmake_parse_arguments(
        ARGS
        ""
        ""
        "SOURCES;LINK_LIBRARIES"
        ${ARGN})
    add_library(adio-${name} ${ARGS_SOURCES})
    target_link_libraries(adio-${name} PUBLIC adio::adio ${ARGS_LINK_LIBRARIES})
    set_property(TARGET adio-${name} PROPERTY EXPORT_NAME adio::${driver})
    add_library(adio::${name} ALIAS adio-${name})
    target_include_directories(adio-${name} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
endfunction()

add_subdirectory(drivers)
