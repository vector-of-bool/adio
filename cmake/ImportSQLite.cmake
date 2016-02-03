find_library(SQLITE_LIBRARY NAMES libsqlite3.a sqlite3.lib libsqlite3.lib)
find_path(SQLITE_DIR sqlite3.h)

if(NOT SQLITE_LIBRARY)
    message(FATAL_ERROR "Cannot find SQLite")
endif()

add_library(sqlite::sqlite3 STATIC IMPORTED)
set_target_properties(
    sqlite::sqlite3 PROPERTIES
    IMPORTED_LOCATION "${SQLITE_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${SQLITE_DIR}"
    )

if(NOT WIN32)
    set_property(TARGET sqlite::sqlite3 APPEND PROPERTY INTERFACE_LINK_LIBRARIES dl)
endif()
