adio_backend(sqlite
    SOURCES
        adio/sqlite.hpp
        adio/sqlite.cpp
    LINK_LIBRARIES
        sqlite::sqlite3
    )
