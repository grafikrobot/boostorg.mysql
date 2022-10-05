//
// Copyright (c) 2019-2022 Ruben Perez Hidalgo (rubenperez038 at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_MYSQL_DETAIL_NETWORK_ALGORITHMS_EXECUTE_GENERIC_HPP
#define BOOST_MYSQL_DETAIL_NETWORK_ALGORITHMS_EXECUTE_GENERIC_HPP

#include <boost/mysql/error.hpp>
#include <boost/mysql/resultset_base.hpp>
#include <boost/mysql/detail/channel/channel.hpp>
#include <boost/mysql/detail/protocol/resultset_encoding.hpp>


namespace boost {
namespace mysql {
namespace detail {

template <class Stream, class Serializable>
void execute_generic(
    resultset_encoding encoding,
    channel<Stream>& channel,
    const Serializable& request,
    resultset_base& output,
    error_code& err,
    error_info& info
);

template <class Stream, class Serializable, class CompletionToken>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(CompletionToken, void(error_code))
async_execute_generic(
    resultset_encoding encoding,
    channel<Stream>& chan,
    const Serializable& request,
    resultset_base& output,
    error_info& info,
    CompletionToken&& token
);

} // detail
} // mysql
} // boost

#include <boost/mysql/detail/network_algorithms/impl/execute_generic.hpp>

#endif /* INCLUDE_MYSQL_IMPL_NETWORK_ALGORITHMS_READ_RESULTSET_HEAD_HPP_ */
