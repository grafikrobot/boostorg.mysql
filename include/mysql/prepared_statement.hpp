#ifndef INCLUDE_MYSQL_PREPARED_STATEMENT_HPP_
#define INCLUDE_MYSQL_PREPARED_STATEMENT_HPP_

#include "mysql/resultset.hpp"
#include "mysql/impl/channel.hpp"
#include "mysql/impl/messages.hpp"
#include <optional>

namespace mysql
{

constexpr std::array<value, 0> no_statement_params;

template <typename Stream>
class prepared_statement
{
	detail::channel<Stream>* channel_ {};
	detail::com_stmt_prepare_ok_packet stmt_msg_;

	template <typename ForwardIterator>
	void check_num_params(ForwardIterator first, ForwardIterator last, error_code& err, error_info& info) const;
public:
	prepared_statement() = default;
	prepared_statement(detail::channel<Stream>& chan, const detail::com_stmt_prepare_ok_packet& msg) noexcept:
		channel_(&chan), stmt_msg_(msg) {}

	bool valid() const noexcept { return channel_ != nullptr; }
	unsigned id() const noexcept { assert(valid()); return stmt_msg_.statement_id.value; }
	unsigned num_params() const noexcept { assert(valid()); return stmt_msg_.num_params.value; }

	template <typename Collection>
	resultset<Stream> execute(const Collection& params, error_code& err, error_info& info) const
	{
		return execute(std::begin(params), std::end(params), err, info);
	}

	template <typename Collection>
	resultset<Stream> execute(const Collection& params) const
	{
		return execute(std::begin(params), std::end(params));
	}

	template <typename Collection, typename CompletionToken>
	auto async_execute(const Collection& params, CompletionToken&& token) const
	{
		return async_execute(std::begin(params), std::end(params), std::forward<CompletionToken>(token));
	}


	template <typename ForwardIterator>
	resultset<Stream> execute(ForwardIterator params_first, ForwardIterator params_last, error_code&, error_info&) const;


	template <typename ForwardIterator>
	resultset<Stream> execute(ForwardIterator params_first, ForwardIterator params_last) const;

	template <typename ForwardIterator, typename CompletionToken>
	auto async_execute(ForwardIterator params_first, ForwardIterator params_last, CompletionToken&& token) const;
};

using tcp_prepared_statement = prepared_statement<boost::asio::ip::tcp::socket>;

}

#include "mysql/impl/prepared_statement.ipp"

#endif /* INCLUDE_MYSQL_PREPARED_STATEMENT_HPP_ */
