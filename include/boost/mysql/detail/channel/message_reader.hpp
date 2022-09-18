//
// Copyright (c) 2019-2022 Ruben Perez Hidalgo (rubenperez038 at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_MYSQL_DETAIL_CHANNEL_MESSAGE_READER_HPP
#define BOOST_MYSQL_DETAIL_CHANNEL_MESSAGE_READER_HPP

#include <boost/mysql/error.hpp>
#include <boost/mysql/detail/channel/read_buffer.hpp>
#include <boost/mysql/detail/channel/message_parser.hpp>
#include <boost/asio/async_result.hpp>
#include <cassert>
#include <cstddef>
#include <cstdint>


namespace boost {
namespace mysql {
namespace detail {

class message_reader
{
public:
    message_reader(std::size_t initial_buffer_size) : buffer_(initial_buffer_size) {}

    bool has_message() const noexcept { return result_.has_message; }
    const std::uint8_t* buffer_first() const noexcept { return buffer_.first(); }

    inline boost::asio::const_buffer get_next_message(std::uint8_t& seqnum, error_code& ec) noexcept;

    // Reads some messages from stream, until there is at least one
    template <class Stream>
    void read_some(Stream& stream, error_code& ec, bool keep_messages = false);

    template <class Stream, class CompletionToken>
    BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(CompletionToken, void(error_code))
    async_read_some(Stream& stream, CompletionToken&& token, bool keep_messages = false);

    // Exposed for the sake of testing
    read_buffer& buffer() noexcept { return buffer_; }
private:

    template <class Stream>
    struct read_op;

    read_buffer buffer_;
    message_parser parser_;
    message_parser::result result_;

    void parse_message() { parser_.parse_message(buffer_, result_); }
    inline void maybe_remove_processed_messages();
    inline void maybe_resize_buffer();
    inline void on_read_bytes(size_t num_bytes);
};


} // detail
} // mysql
} // boost

#include <boost/mysql/detail/channel/impl/message_reader.hpp>

#endif /* INCLUDE_BOOST_MYSQL_DETAIL_AUXILIAR_STATIC_STRING_HPP_ */




