//
// Copyright (c) 2019-2020 Ruben Perez Hidalgo (rubenperez038 at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "integration_test_common.hpp"

using namespace boost::mysql::test;
using boost::mysql::detail::make_error_code;
using boost::mysql::field_metadata;
using boost::mysql::field_type;
using boost::mysql::error_code;
using boost::mysql::error_info;
using boost::mysql::ssl_mode;
using boost::mysql::connection;
using boost::mysql::resultset;
using boost::mysql::prepared_statement;
using boost::mysql::row;
using boost::mysql::owning_row;
namespace net = boost::asio;

BOOST_AUTO_TEST_SUITE(test_resultset)

// Helpers
template <typename... Types>
std::vector<owning_row> makerows(std::size_t row_size, Types&&... args)
{
    auto values = make_value_vector(std::forward<Types>(args)...);
    assert(values.size() % row_size == 0);
    std::vector<owning_row> res;
    for (std::size_t i = 0; i < values.size(); i += row_size)
    {
        std::vector<boost::mysql::value> row_values (
            values.begin() + i, values.begin() + i + row_size);
        res.push_back(owning_row(std::move(row_values), {}));
    }
    return res;
}

bool operator==(const std::vector<owning_row>& lhs, const std::vector<owning_row>& rhs)
{
    return boost::mysql::detail::container_equals(lhs, rhs);
}

template <typename Stream>
void validate_eof(
    const resultset<Stream>& result,
    int affected_rows=0,
    int warnings=0,
    int last_insert=0,
    boost::string_view info=""
)
{
    BOOST_TEST_REQUIRE(result.valid());
    BOOST_TEST_REQUIRE(result.complete());
    BOOST_TEST(result.affected_rows() == affected_rows);
    BOOST_TEST(result.warning_count() == warnings);
    BOOST_TEST(result.last_insert_id() == last_insert);
    BOOST_TEST(result.info() == info);
}

// Interface to generate a resultset
template <typename Stream>
class resultset_generator
{
public:
    virtual ~resultset_generator() {}
    virtual const char* name() const = 0;
    virtual resultset<Stream> generate(connection<Stream>&, boost::string_view) = 0;
};

template <typename Stream>
class text_resultset_generator : public resultset_generator<Stream>
{
public:
    const char* name() const override { return "text"; }
    resultset<Stream> generate(connection<Stream>& conn, boost::string_view query) override
    {
        return conn.query(query);
    }
};

template <typename Stream>
class binary_resultset_generator : public resultset_generator<Stream>
{
public:
    const char* name() const override { return "binary"; }
    resultset<Stream> generate(connection<Stream>& conn, boost::string_view query) override
    {
        return conn.prepare_statement(query).execute(boost::mysql::no_statement_params);
    }
};

// Sample type
template <typename Stream>
struct resultset_sample
{
    network_functions<Stream>* net;
    ssl_mode ssl;
    resultset_generator<Stream>* gen;

    resultset_sample(network_functions<Stream>* funs, ssl_mode ssl, resultset_generator<Stream>* gen):
        net(funs),
        ssl(ssl),
        gen(gen)
    {
    }

    void set_test_attributes(boost::unit_test::test_case& test) const
    {
        test.add_label(net->name());
        test.add_label(to_string(ssl));
    }
};

template <typename Stream>
std::ostream& operator<<(std::ostream& os, const resultset_sample<Stream>& input)
{
    return os << input.net->name() << '_'
              << to_string(input.ssl)
              << '_' << input.gen->name();
}

struct sample_gen
{
    template <typename Stream>
    static std::vector<resultset_sample<Stream>> make_all()
    {
        static text_resultset_generator<Stream> text_obj;
        static binary_resultset_generator<Stream> binary_obj;

        resultset_generator<Stream>* all_resultset_generators [] = {
            &text_obj,
            &binary_obj
        };
        ssl_mode all_ssl_modes [] = {
            ssl_mode::disable,
            ssl_mode::require
        };

        std::vector<resultset_sample<Stream>> res;
        for (auto* net: all_network_functions<Stream>())
        {
            for (auto ssl: all_ssl_modes)
            {
                for (auto* gen: all_resultset_generators)
                {
                    res.emplace_back(net, ssl, gen);
                }
            }
        }
        return res;
    }

    template <typename Stream>
    static const std::vector<resultset_sample<Stream>>& generate()
    {
        static auto res = make_all<Stream>();
        return res;
    }
};

BOOST_AUTO_TEST_SUITE(fetch_one)

BOOST_MYSQL_NETWORK_TEST(no_results, network_fixture, sample_gen)
{
    this->connect(sample.ssl);
    auto result = sample.gen->generate(this->conn,
        "SELECT * FROM empty_table");
    BOOST_TEST(result.valid());
    BOOST_TEST(!result.complete());
    BOOST_TEST(result.fields().size() == 2);

    // Already in the end of the resultset, we receive the EOF
    auto row_result = sample.net->fetch_one(result);
    row_result.validate_no_error();
    BOOST_TEST(row_result.value == nullptr);
    validate_eof(result);

    // Fetching again just returns null
    row_result = sample.net->fetch_one(result);
    row_result.validate_no_error();
    BOOST_TEST(row_result.value == nullptr);
    validate_eof(result);
}

BOOST_MYSQL_NETWORK_TEST(one_row, network_fixture, sample_gen)
{
    this->connect(sample.ssl);
    auto result = sample.gen->generate(this->conn,
        "SELECT * FROM one_row_table");
    BOOST_TEST(result.valid());
    BOOST_TEST(!result.complete());
    BOOST_TEST(result.fields().size() == 2);

    // Fetch only row
    auto row_result = sample.net->fetch_one(result);
    row_result.validate_no_error();
    BOOST_TEST_REQUIRE(row_result.value != nullptr);
    this->validate_2fields_meta(result, "one_row_table");
    BOOST_TEST((*row_result.value == makerow(1, "f0")));
    BOOST_TEST(!result.complete());

    // Fetch next: end of resultset
    row_result = sample.net->fetch_one(result);
    row_result.validate_no_error();
    BOOST_TEST(row_result.value == nullptr);
    validate_eof(result);
}

BOOST_MYSQL_NETWORK_TEST(two_rows, network_fixture, sample_gen)
{
    this->connect(sample.ssl);
    auto result = sample.gen->generate(this->conn,
        "SELECT * FROM two_rows_table");
    BOOST_TEST(result.valid());
    BOOST_TEST(!result.complete());
    BOOST_TEST(result.fields().size() == 2);

    // Fetch first row
    auto row_result = sample.net->fetch_one(result);
    row_result.validate_no_error();
    BOOST_TEST_REQUIRE(row_result.value != nullptr);
    this->validate_2fields_meta(result, "two_rows_table");
    BOOST_TEST((*row_result.value == makerow(1, "f0")));
    BOOST_TEST(!result.complete());

    // Fetch next row
    row_result = sample.net->fetch_one(result);
    row_result.validate_no_error();
    BOOST_TEST_REQUIRE(row_result.value != nullptr);
    this->validate_2fields_meta(result, "two_rows_table");
    BOOST_TEST(*row_result.value == makerow(2, "f1"));
    BOOST_TEST(!result.complete());

    // Fetch next: end of resultset
    row_result = sample.net->fetch_one(result);
    row_result.validate_no_error();
    BOOST_TEST(row_result.value == nullptr);
    validate_eof(result);
}

// There seems to be no real case where fetch can fail (other than net fails)

BOOST_AUTO_TEST_SUITE_END() // fetch_one

BOOST_AUTO_TEST_SUITE(fetch_many)

BOOST_MYSQL_NETWORK_TEST(no_results, network_fixture, sample_gen)
{
    this->connect(sample.ssl);
    auto result = sample.gen->generate(this->conn,
        "SELECT * FROM empty_table");

    // Fetch many, but there are no results
    auto rows_result = sample.net->fetch_many(result, 10);
    rows_result.validate_no_error();
    BOOST_TEST(rows_result.value.empty());
    validate_eof(result);

    // Fetch again, should return OK and empty
    rows_result = sample.net->fetch_many(result, 10);
    rows_result.validate_no_error();
    BOOST_TEST(rows_result.value.empty());
    validate_eof(result);
}

BOOST_MYSQL_NETWORK_TEST(more_rows_than_count, network_fixture, sample_gen)
{
    this->connect(sample.ssl);
    auto result = sample.gen->generate(this->conn,
        "SELECT * FROM three_rows_table");

    // Fetch 2, one remaining
    auto rows_result = sample.net->fetch_many(result, 2);
    rows_result.validate_no_error();
    BOOST_TEST(!result.complete());
    BOOST_TEST((rows_result.value == makerows(2, 1, "f0", 2, "f1")));

    // Fetch another two (completes the resultset)
    rows_result = sample.net->fetch_many(result, 2);
    rows_result.validate_no_error();
    validate_eof(result);
    BOOST_TEST((rows_result.value == makerows(2, 3, "f2")));
}

BOOST_MYSQL_NETWORK_TEST(less_rows_than_count, network_fixture, sample_gen)
{
    this->connect(sample.ssl);
    auto result = sample.gen->generate(this->conn,
        "SELECT * FROM two_rows_table");

    // Fetch 3, resultset exhausted
    auto rows_result = sample.net->fetch_many(result, 3);
    rows_result.validate_no_error();
    BOOST_TEST((rows_result.value == makerows(2, 1, "f0", 2, "f1")));
    validate_eof(result);
}

BOOST_MYSQL_NETWORK_TEST(same_rows_as_count, network_fixture, sample_gen)
{
    this->connect(sample.ssl);
    auto result = sample.gen->generate(this->conn,
        "SELECT * FROM two_rows_table");

    // Fetch 2, 0 remaining but resultset not exhausted
    auto rows_result = sample.net->fetch_many(result, 2);
    rows_result.validate_no_error();
    BOOST_TEST(!result.complete());
    BOOST_TEST((rows_result.value == makerows(2, 1, "f0", 2, "f1")));

    // Fetch again, exhausts the resultset
    rows_result = sample.net->fetch_many(result, 2);
    rows_result.validate_no_error();
    BOOST_TEST(rows_result.value.empty());
    validate_eof(result);
}

BOOST_MYSQL_NETWORK_TEST(count_equals_one, network_fixture, sample_gen)
{
    this->connect(sample.ssl);
    auto result = sample.gen->generate(this->conn,
        "SELECT * FROM one_row_table");

    // Fetch 1, 1 remaining
    auto rows_result = sample.net->fetch_many(result, 1);
    rows_result.validate_no_error();
    BOOST_TEST(!result.complete());
    BOOST_TEST((rows_result.value == makerows(2, 1, "f0")));
}

BOOST_AUTO_TEST_SUITE_END() // fetch_many

BOOST_AUTO_TEST_SUITE(fetch_all)

BOOST_MYSQL_NETWORK_TEST(no_results, network_fixture, sample_gen)
{
    this->connect(sample.ssl);
    auto result = sample.gen->generate(this->conn,
        "SELECT * FROM empty_table");

    // Fetch many, but there are no results
    auto rows_result = sample.net->fetch_all(result);
    rows_result.validate_no_error();
    BOOST_TEST(rows_result.value.empty());
    BOOST_TEST(result.complete());

    // Fetch again, should return OK and empty
    rows_result = sample.net->fetch_all(result);
    rows_result.validate_no_error();
    BOOST_TEST(rows_result.value.empty());
    validate_eof(result);
}

BOOST_MYSQL_NETWORK_TEST(one_row, network_fixture, sample_gen)
{
    this->connect(sample.ssl);
    auto result = sample.gen->generate(this->conn,
        "SELECT * FROM one_row_table");

    auto rows_result = sample.net->fetch_all(result);
    rows_result.validate_no_error();
    BOOST_TEST(result.complete());
    BOOST_TEST((rows_result.value == makerows(2, 1, "f0")));
}

BOOST_MYSQL_NETWORK_TEST(several_rows, network_fixture, sample_gen)
{
    this->connect(sample.ssl);
    auto result = sample.gen->generate(this->conn,
        "SELECT * FROM two_rows_table");

    auto rows_result = sample.net->fetch_all(result);
    rows_result.validate_no_error();
    validate_eof(result);
    BOOST_TEST(result.complete());
    BOOST_TEST((rows_result.value == makerows(2, 1, "f0", 2, "f1")));
}

BOOST_AUTO_TEST_SUITE_END() // fetch_all

BOOST_AUTO_TEST_SUITE_END() // test_resultset

