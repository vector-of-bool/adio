add_library(adio-core STATIC)
add_library(adio::core ALIAS adio-core)
export_library(adio-core NAME core HEADER_ROOTS src/)

file(GLOB_RECURSE lib_sources CONFIGURE_DEPENDS src/*)
file(GLOB_RECURSE test_sources CONFIGURE_DEPENDS src/*.test.cpp)
list(REMOVE_ITEM lib_sources ${test_sources} ~~~)
target_sources(adio-core PRIVATE ${lib_sources})

target_compile_definitions(adio-core PUBLIC
    ADIO_BACKEND=${ADIO_BACKEND}
    ADIO_CONFIG=$<LOWER_CASE:$<CONFIG>>
    )

target_include_directories(adio-core PUBLIC src)
target_link_libraries(adio-core PUBLIC Boost::asio Threads::Threads)
target_compile_features(adio-core PUBLIC cxx_std_17)

# Add tests for each source we found
foreach(test_fpath IN LISTS test_sources)
    get_filename_component(test_name "${test_fpath}" NAME_WE)
    file(RELATIVE_PATH relpath "${PROJECT_SOURCE_DIR}" "${test_fpath}")
    get_filename_component(reldir "${relpath}" DIRECTORY)
    add_executable("test-${test_name}" "${test_fpath}")
    target_link_libraries("test-${test_name}" PRIVATE adio::core catch-main catch2::catch2)
    catch_discover_tests(test-${test_name} TEST_PREFIX "${reldir}/${test_name}::")
endforeach()
