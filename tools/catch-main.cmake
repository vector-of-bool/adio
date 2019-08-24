get_filename_component(catch_mainfile "${CMAKE_CURRENT_BINARY_DIR}/_catch-main.cpp" ABSOLUTE)
if(NOT EXISTS "${catch_mainfile}")
    file(WRITE "${catch_mainfile}" [[
        #define CATCH_CONFIG_MAIN
        #include <catch2/catch.hpp>
    ]])
endif()
add_library(catch-main OBJECT "${catch_mainfile}")
target_link_libraries(catch-main PRIVATE Catch2::Catch2)
