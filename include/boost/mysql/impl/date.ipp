//
// Copyright (c) 2019-2023 Ruben Perez Hidalgo (rubenperez038 at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_MYSQL_IMPL_DATE_IPP
#define BOOST_MYSQL_IMPL_DATE_IPP

#pragma once

#include <boost/mysql/date.hpp>
#include <boost/mysql/string_view.hpp>

#include <boost/mysql/detail/access.hpp>

#include <boost/mysql/impl/internal/dt_to_string.hpp>

#include <cstddef>
#include <ostream>

std::size_t boost::mysql::date::impl_t::to_string(span<char, 32> output) const noexcept
{
    return detail::date_to_string(year, month, day, output);
}

std::ostream& boost::mysql::operator<<(std::ostream& os, const date& value)
{
    char buffer[32]{};
    std::size_t sz = detail::access::get_impl(value).to_string(buffer);
    return os << string_view(buffer, sz);
}

#endif