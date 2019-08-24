
import conans

LIBMAN_DEP = 'libman/0.2.0@test/test'


class ConanFile(conans.python_requires(LIBMAN_DEP).CMakeConanFile):
    name = 'adio'
    version = '0.0.1'
    description = 'Database abstraction library build on Asio'

    requires = 'boost/1.69.0@vector-of-bool/libman-test'

    build_requires = (
        'catch2/2.3.0@bincrafters/stable',
        LIBMAN_DEP,
    )

    exports_sources = '*', '!build/*'
    generators = 'LibMan', 'cmake'
    libman_for = 'cmake'
    no_copy_source = True
