find_library(PQXX_LIBRARY NAMES libpqxx.a pqxx.lib)
find_path(PQXX_INCLUDE_DIRECTORY pqxx/pqxx)

if(NOT PQXX_LIBRARY)
    message(WARNING_AUTHOR "Did not find libpqxx for PostgreSQL support")
    return()
endif()

if(NOT PQXX_INCLUDE_DIRECTORY)
    message(WARNING_AUTHOR "Did not find pqxx include directories")
    return()
endif()

adio_backend(postgresql
    SOURCES
        adio/postgresql.hpp
        adio/postgresql.cpp
    LINK_LIBRARIES ${PQXX_LIBRARY}
    )
