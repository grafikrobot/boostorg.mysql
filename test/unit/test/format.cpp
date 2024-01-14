//
// Copyright (c) 2019-2023 Ruben Perez Hidalgo (rubenperez038 at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/mysql/character_set.hpp>
#include <boost/mysql/field_view.hpp>
#include <boost/mysql/format.hpp>
#include <boost/mysql/string_view.hpp>

#include <boost/test/unit_test.hpp>

#include <string>
#ifdef __cpp_lib_string_view
#include <string_view>
#endif

using namespace boost::mysql;

/**
 * identifier
 *    all empty
 *    single field
 *    double field
 *    triple field
 *    escaped correctly
 * adding a custom field
 * injection attempts
 *    strings are escaped correctly
 *    error thrown on invalid encoding
 *    {} in strings doesn't have any meaning
 *    {{}} in strings doesn't have any meaning
 * format strings
 *    empty string
 *    no replacement
 *    escaped {{}}
 *    unbalanced escaped {{}}
 *    text {}
 *    {} text
 *    {}
 *    {}{}
 *    text {} text
 *    {} text {}
 *    text {} text {} text
 *    {}{}{}
 *    {named}{named2}
 *    {named} // unused name
 *    {Stran_es0t_name}
 *    {1}{0}
 *    {0}{0} // repeated
 *    {1}{2} // unused arg
 *    {}{}   // unused arg
 *    {071} // interpreted as 71, not octal
 *    {named}{1}{named2}{2} // mix
 *    {named}{} // is this allowed???
 *    {{{}}} // this is OK
 * invalid format strings
 *    unbalanced { text
 *    unbalanced { // end of string
 *    unbalanced } text
 *    unbalanced } // end of string
 *    {0name} // name starting with a number
 *    {!name"} // name containing invalid ascii
 *    { name } // spaces not allowed
 *    {ñ} // name containing non-ascii
 *    {valid_name!} // extra chars not allowed
 *    {valid_name:} // formatting options not allowed
 *    {:} // formatting options not allowed
 *    {0xab} // hex not allowed
 *    { 1 } // spaces not allowed
 *    {45abc} // extra trailing chars not allowed
 *    {}} // unbalanced }
 *    {0:} // formatting options not allowed
 *    {}{0} // cannot switch from auto to manual
 *    {0}{} // cannot swith from manual to auto
 *    {name // end of string while scanning name
 *    {21 // end of string while scanning number
 *    { {} } // replacement field inside replacement field
 * arg/strings invalid combinations
 *    auto indexing out of range
 *    manual indexing out of range
 *    name not found
 * charset
 *    multi-byte characters allowed in format strings
 *    invalid multi-byte characters in format strings throw
 *    multi-byte characters with { continuation cause no problems
 *    multi-byte characters with } continuation cause no problems
 * the string is cleared on input
 * backslash_escapes is correctly propagated to escaping
 * format_context
 *    the string is not cleared when created
 *    can be created from strings that are not std::string (TODO: we should test this in other places, too)
 *    add_raw just appends, even if it has special chars
 *    add_value with basic types
 *    add_value with writable fields like optionals
 *    add_value with custom formatters
 *    neither add_raw not add_value clear the string
 * float, double: isnan, isinf
 */

BOOST_AUTO_TEST_SUITE(test_format)

constexpr format_options opts{utf8mb4_charset, true};
constexpr auto single_fmt = "SELECT {};";

class locale_guard
{
    std::string old_locale_;

public:
    locale_guard(const char* locale) : old_locale_(std::setlocale(LC_ALL, locale)) {}
    locale_guard(const locale_guard&) = delete;
    locale_guard& operator=(locale_guard&) = delete;
    ~locale_guard() { std::setlocale(LC_ALL, old_locale_.c_str()); }
};

BOOST_AUTO_TEST_CASE(individual_fields)
{
    // This operation shouldn't be affected by locale
    locale_guard guard("en_DK.utf8");

    // null
    BOOST_TEST(format(single_fmt, opts, nullptr) == "SELECT NULL;");

    // signed char
    BOOST_TEST(format(single_fmt, opts, (signed char)42) == "SELECT 42;");
    BOOST_TEST(format(single_fmt, opts, (signed char)-1) == "SELECT -1;");
    BOOST_TEST(format(single_fmt, opts, (signed char)127) == "SELECT 127;");

    // unsigned char
    BOOST_TEST(format(single_fmt, opts, (unsigned char)42) == "SELECT 42;");
    BOOST_TEST(format(single_fmt, opts, (unsigned char)0) == "SELECT 0;");
    BOOST_TEST(format(single_fmt, opts, (unsigned char)0xff) == "SELECT 255;");

    // signed short
    BOOST_TEST(format(single_fmt, opts, (short)42) == "SELECT 42;");
    BOOST_TEST(format(single_fmt, opts, (short)-1) == "SELECT -1;");
    BOOST_TEST(format(single_fmt, opts, (short)-32768) == "SELECT -32768;");

    // unsigned short
    BOOST_TEST(format(single_fmt, opts, (unsigned short)42) == "SELECT 42;");
    BOOST_TEST(format(single_fmt, opts, (unsigned short)0) == "SELECT 0;");
    BOOST_TEST(format(single_fmt, opts, (unsigned short)0xffff) == "SELECT 65535;");

    // signed int
    BOOST_TEST(format(single_fmt, opts, (int)42) == "SELECT 42;");
    BOOST_TEST(format(single_fmt, opts, (int)-1) == "SELECT -1;");
    BOOST_TEST(format(single_fmt, opts, (int)0x7fffffff) == "SELECT 2147483647;");

    // unsigned int
    BOOST_TEST(format(single_fmt, opts, (unsigned int)42) == "SELECT 42;");
    BOOST_TEST(format(single_fmt, opts, (unsigned int)0) == "SELECT 0;");
    BOOST_TEST(format(single_fmt, opts, (unsigned int)0xffffffff) == "SELECT 4294967295;");

    // signed long
    BOOST_TEST(format(single_fmt, opts, (long)42) == "SELECT 42;");
    BOOST_TEST(format(single_fmt, opts, (long)-1) == "SELECT -1;");
    BOOST_TEST(format(single_fmt, opts, (long)0x7fffffff) == "SELECT 2147483647;");

    // unsigned long
    BOOST_TEST(format(single_fmt, opts, (unsigned long)42) == "SELECT 42;");
    BOOST_TEST(format(single_fmt, opts, (unsigned long)0) == "SELECT 0;");
    BOOST_TEST(format(single_fmt, opts, (unsigned long)0xffffffff) == "SELECT 4294967295;");

    // signed long long
    BOOST_TEST(format(single_fmt, opts, (long long)42) == "SELECT 42;");
    BOOST_TEST(format(single_fmt, opts, (long long)-1) == "SELECT -1;");
    BOOST_TEST(format(single_fmt, opts, (long long)0x7fffffffffffffff) == "SELECT 9223372036854775807;");

    // unsigned long long
    BOOST_TEST(format(single_fmt, opts, (unsigned long long)42) == "SELECT 42;");
    BOOST_TEST(format(single_fmt, opts, (unsigned long long)0) == "SELECT 0;");
    BOOST_TEST(
        format(single_fmt, opts, (unsigned long long)0xffffffffffffffff) == "SELECT 18446744073709551615;"
    );

    // bool
    BOOST_TEST(format(single_fmt, opts, true) == "SELECT 1;");
    BOOST_TEST(format(single_fmt, opts, false) == "SELECT 0;");

    // float
    BOOST_TEST(format(single_fmt, opts, 4.2f) == "SELECT 4.199999809265137e+00;");

    // double
    BOOST_TEST(format(single_fmt, opts, 4.2) == "SELECT 4.2e+00;");

    // string literals
    BOOST_TEST(format(single_fmt, opts, "abc") == "SELECT 'abc';");
    BOOST_TEST(format(single_fmt, opts, "abc'\\ OR 1=1") == "SELECT 'abc\\'\\\\ OR 1=1';");
    BOOST_TEST(format(single_fmt, opts, "hola \xc3\xb1!") == "SELECT 'hola \xc3\xb1!';");
    BOOST_TEST(format(single_fmt, opts, "") == "SELECT '';");

    // C strings
    BOOST_TEST(format(single_fmt, opts, static_cast<const char*>("abc")) == "SELECT 'abc';");
    BOOST_TEST(format(single_fmt, opts, static_cast<const char*>("")) == "SELECT '';");

    // std::string
    std::string lval = "I'm an lvalue";
    const std::string clval = "I'm const";
    BOOST_TEST(format(single_fmt, opts, lval) == "SELECT 'I\\'m an lvalue';");
    BOOST_TEST(format(single_fmt, opts, clval) == "SELECT 'I\\'m const';");
    BOOST_TEST(format(single_fmt, opts, std::string("abc")) == "SELECT 'abc';");
    BOOST_TEST(format(single_fmt, opts, std::string()) == "SELECT '';");

    // string_view
    BOOST_TEST(format(single_fmt, opts, string_view("abc")) == "SELECT 'abc';");
    BOOST_TEST(format(single_fmt, opts, string_view("abc'\\ OR 1=1")) == "SELECT 'abc\\'\\\\ OR 1=1';");
    BOOST_TEST(format(single_fmt, opts, string_view()) == "SELECT '';");

// std::string_view
#ifdef __cpp_lib_string_view
    BOOST_TEST(format(single_fmt, opts, std::string_view("abc")) == "SELECT 'abc';");
    BOOST_TEST(format(single_fmt, opts, std::string_view("abc'\\ OR 1=1")) == "SELECT 'abc\\'\\\\ OR 1=1';");
    BOOST_TEST(format(single_fmt, opts, std::string_view()) == "SELECT '';");
#endif
}

//  *    blob, blob_view
//  *    date, time, datetime
//  *    field, field_view
//  *    boost::optional, std::optional
//  *    raw

// doubles have many different cases that may cause trouble, so we
// have a separate test case for them. floats are formatted as doubles, so don't need extra coverage
BOOST_AUTO_TEST_CASE(individual_double)
{
    struct
    {
        string_view name;
        double value;
        string_view expected;
    } test_cases[] = {
        {"regular",               4.2,                      "4.2e+00"                 },
        {"regular_precision",     4.298238239237823287327,  "4.298238239237823e+00"   },
        {"exp",                   5.1e+23,                  "5.1e+23"                 },
        {"exp_precision",         4.2982382392378232e+67,   "4.2982382392378234e+67"  },
        {"max",                   1.7976931348623157e+308,  "1.7976931348623157e+308" },
        {"regular_neg",           -4.2,                     "-4.2e+00"                },
        {"regular_precision_neg", -4.298238239237823287327, "-4.298238239237823e+00"  },
        {"exp_neg",               -5.1e+23,                 "-5.1e+23"                },
        {"max_neg",               -1.7976931348623157e+308, "-1.7976931348623157e+308"},
        {"zero",                  0.0,                      "0e+00"                   },
        {"zero_neg",              -0.0,                     "-0e+00"                  },
        {"expneg",                4.2e-12,                  "4.2e-12"                 },
        {"expneg_precision",      4.2872383293922839e-45,   "4.2872383293922836e-45"  },
        {"min",                   2.2250738585072014e-308,  "2.2250738585072014e-308" },
        {"min_neg",               -2.2250738585072014e-308, "-2.2250738585072014e-308"},
        {"denorm",                -4.2872383293922839e-309, "-4.287238329392283e-309" },
        {"min_denorm",            5e-324,                   "5e-324"                  },
    };

    for (const auto& tc : test_cases)
    {
        BOOST_TEST_CONTEXT(tc.name)
        {
            auto str = format("{}", opts, tc.value);
            BOOST_TEST(str == tc.expected);
        }
    }
}

// BOOST_TEST(
//     format("SELECT * FROM myt WHERE id = {}", default_opts(), 42) == "SELECT * FROM myt WHERE id = 42"
// );
// BOOST_TEST(format("SELECT * FROM {}", default_opts(), identifier("my`tab")) == "SELECT * FROM `my``tab`");
// BOOST_TEST(
//     format("SELECT * FROM {}", default_opts(), identifier("my`db", "my`tab")) ==
//     "SELECT * FROM `my``db`.`my``tab`"
// );

BOOST_AUTO_TEST_SUITE_END()
