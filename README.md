# Adio
## Asynchronous Database Library based on Asio

Adio is a database abstraction library built on top of Asio. Adio code may look
very similar to code in Asio, and it follows the same patterns and styles found
in Asio. For example, to open a database connection synchronously:

~~~cpp
#include <adio/sqlite.hpp>

int main()
{
    adio::io_service ios;
    adio::sqlite::connection con{ ios };
    const auto ec = con.open("database.db");
    if (!ec)
        std::cout << "Database was opened!\n";
    else
        std::cout << "There was an error!\n";
}
~~~

And asynchronously:

~~~cpp
#include <adio/sqlite.hpp>

int main()
{
    adio::io_service ios;
    adio::sqlite::connection con{ ios };
    con.async_open("database.db", [&con](const auto ec) {
        if (!ec)
            std::cout << "Database was opened!\n";
        else
            std::cout << "There was an error!\n";
    });
    ios.run();
}
~~~

\* Note that ``adio::io_service`` is simply an alias to ``asio::io_service`` or
``boost::asio::io_service``, depending on the Asio backend selected.

## Important note:

Adio is very much in early developement. Feedback would be much appreciated!