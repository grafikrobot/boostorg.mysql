//
// Copyright (c) 2019-2023 Ruben Perez Hidalgo (rubenperez038 at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/describe/class.hpp>

#ifdef BOOST_DESCRIBE_CXX14

//[example_batch_inserts

// Uses client-side SQL formatting to implement a batch inserts
// for a specific type. It makes use of format_context.
// The program reads a JSON file containing a list of employees
// and inserts it into the employee table.
//
// This example requires C++14 to work because it uses Boost.Describe
// to simplify JSON parsing. All Boost.MySQL features used are C++11 compatible.
//
// Note: client-side SQL formatting is an experimental feature.

#include <boost/mysql/any_connection.hpp>
#include <boost/mysql/character_set.hpp>
#include <boost/mysql/error_code.hpp>
#include <boost/mysql/error_with_diagnostics.hpp>
#include <boost/mysql/format_sql.hpp>
#include <boost/mysql/results.hpp>
#include <boost/mysql/string_view.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/core/span.hpp>
#include <boost/describe/class.hpp>
#include <boost/describe/members.hpp>
#include <boost/describe/modifiers.hpp>
#include <boost/json/parse.hpp>
#include <boost/json/value_to.hpp>

#include <fstream>
#include <iostream>
#include <string>

using boost::mysql::error_code;
using boost::mysql::string_view;

/**
 * We will use Boost.Describe to easily parse the JSON file
 * into a std::vector<employee>. The JSON file contain an array
 * of objects like the following:
 * {
 *     "first_name": "Some string",
 *     "last_name": "Some other string",
 *     "company_id": "String",
 *     "salary": 20000
 * }
 */
struct employee
{
    std::string first_name;
    std::string last_name;
    std::string company_id;
    double salary;
};

// Adds reflection capabilities to employee. Required by the JSON parser.
BOOST_DESCRIBE_STRUCT(employee, (), (first_name, last_name, company_id, salary))

// Reads a file into memory
static std::string read_file(const char* file_name)
{
    std::ifstream ifs(file_name);
    if (!ifs)
        throw std::runtime_error("Cannot open file: " + std::string(file_name));
    return std::string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
}

// Composes an INSERT SQL query suitable to be sent to the server.
// For instance, when inserting two employees, something like the following may be generated:
// INSERT INTO employee (first_name, last_name, company_id, salary)
//     VALUES ('John', 'Doe', 'HGS', 20000), ('Rick', 'Smith', 'LLC', 50000)
static std::string compose_batch_insert(
    // Connection config options required for the formatting.
    // This includes the character set currently in use.
    boost::mysql::format_options opts,

    // The list of employees to insert, as read from the JSON file
    const std::vector<employee>& employees
)
{
    // A format_context accumulates our query as we build it
    boost::mysql::format_context ctx(opts);

    // format_context::append_raw adds raw SQL to the output
    ctx.append_raw("INSERT INTO employee (first_name, last_name, company_id, salary) VALUES ");

    // Iterate over all employees, building a comma-separated list of values to insert
    bool is_first = true;
    for (const auto& emp : employees)
    {
        if (!is_first)
            ctx.append_raw(", ");

        // append_value adds a SQL literal with the right format.
        // For strings, adds an escaped, quoted string literal.
        // For doubles, it adds a number literal.
        ctx.append_raw("(")
            .append_value(emp.first_name)
            .append_raw(", ")
            .append_value(emp.last_name)
            .append_raw(", ")
            .append_value(emp.company_id)
            .append_raw(", ")
            .append_value(emp.salary)
            .append_raw(")");
        is_first = false;
    }

    // fomat_context::get() returns the result of our formatting operation.
    // Formatting can fail (e.g. if you supply strings with invalid UTF-8),
    // so get() returns a boost::system::result<std::string>.
    // Calling value() will retrieve the string or throw an exception on failure.
    return ctx.get().value();
}

void main_impl(int argc, char** argv)
{
    if (argc != 5)
    {
        std::cerr << "Usage: " << argv[0] << " <username> <password> <server-hostname> <input-file>\n";
        exit(1);
    }

    // Read our JSON file into memory
    auto contents = read_file(argv[4]);

    // Parse the JSON. json::parse parses the string into a DOM,
    // and json::value_to validates the JSON schema, parsing values into employee structures
    auto values = boost::json::value_to<std::vector<employee>>(boost::json::parse(contents));

    // Create an I/O context, required by all I/O objects
    boost::asio::io_context ctx;

    // Create a connection. Note that client-side SQL formatting
    // requires us to use the newer any_connection.
    boost::mysql::any_connection conn(ctx);

    // Connection configuration. By default, connections use the utf8mb4 character set
    // (MySQL's name for regular UTF-8).
    boost::mysql::connect_params params;
    params.server_address.emplace_host_and_port(argv[3]);
    params.username = argv[1];
    params.password = argv[2];
    params.database = "boost_mysql_examples";

    // Connect to the server
    conn.connect(params);

    // Compose the query. format_opts() returns a system::result<format_options>,
    // containing the options required by format_context. format_opts() may return
    // an error if the connection doesn't know which character set is using -
    // use set_character_set if this happens.
    std::string query = compose_batch_insert(conn.format_opts().value(), values);

    // Execute the query as normal. Note that, unlike with prepared statements,
    // formatting happened in the client, and not in the server.
    boost::mysql::results result;
    conn.execute(query, result);
    std::cout << "Done\n";

    // Notify the MySQL server we want to quit, then close the underlying connection.
    conn.close();
}

int main(int argc, char** argv)
{
    try
    {
        main_impl(argc, argv);
    }
    catch (const boost::mysql::error_with_diagnostics& err)
    {
        // Some errors include additional diagnostics, like server-provided error messages.
        // Security note: diagnostics::server_message may contain user-supplied values (e.g. the
        // field value that caused the error) and is encoded using to the connection's character set
        // (UTF-8 by default). Treat is as untrusted input.
        std::cerr << "Error: " << err.what() << '\n'
                  << "Server diagnostics: " << err.get_diagnostics().server_message() << std::endl;
        return 1;
    }
    catch (const std::exception& err)
    {
        std::cerr << "Error: " << err.what() << std::endl;
        return 1;
    }
}

//]

#else

#include <iostream>

int main()
{
    std::cout << "Sorry, your compiler doesn't have the required capabilities to run this example"
              << std::endl;
}

#endif
