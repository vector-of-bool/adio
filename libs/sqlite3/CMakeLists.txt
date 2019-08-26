adio_submodule(sqlite3 LINKS PUBLIC adio::core)

# Configure the underlying SQLite library
set_property(
    SOURCE src/adio/drivers/sqlite3/third/sqlite3.c
    PROPERTY COMPILE_DEFINITIONS
        # Set the threading mode to multi-threaded, not serialized nor single-threaded
        SQLITE_THREADSAFE=2
        # Disable extension loading so we have no requirement on ::dlopen
        SQLITE_OMIT_LOAD_EXTENSION=1
    )
