#ifndef FOXY_CLIENT_SESSION_HPP_
#define FOXY_CLIENT_SESSION_HPP_

#include <memory>
#include <utility>
#include <string_view>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/associated_executor.hpp>

#include <boost/system/error_code.hpp>

#include <boost/beast/core/flat_buffer.hpp>

#include "foxy/coroutine.hpp"

namespace foxy {

struct client_session {
public:
  using timer_type  = boost::asio::steady_timer;
  using buffer_type = boost::beast::flat_buffer;
  using stream_type = boost::asio::ip::tcp::socket;
  using strand_type =
    boost::asio::strand<boost::asio::io_context::executor_type>;

private:
  struct session_state {
    timer_type  timer;
    buffer_type buffer;
    stream_type stream;
    strand_type strand;

    session_state(boost::asio::io_context& io)
    : timer(io)
    , stream(io)
    , strand(stream.get_executor())
    {
    }
  };

  std::shared_ptr<session_state> s_;

public:
  client_session()                      = delete;
  client_session(client_session const&) = default;
  client_session(client_session&&)      = default;

  explicit
  client_session(boost::asio::io_context& io)
  : s_(std::make_shared<session_state>(io))
  {
  }

  template <typename CompletionToken>
  auto async_connect(
    std::string_view const host,
    std::string_view const service,
    CompletionToken&&      token
  ) & -> BOOST_ASIO_INITFN_RESULT_TYPE(
    CompletionToken,
    void(boost::system::error_code, boost::asio::ip::tcp::endpoint const&)
  ) {
    namespace asio = boost::asio;
    using asio::ip::tcp;

    asio::async_completion<
      CompletionToken,
      void(boost::system::error_code, boost::asio::ip::tcp::endpoint const&)
    >
    init(token);

    auto executor = asio::get_associated_executor(
      init.completion_handler, s_->stream.get_executor());

    co_spawn(
      executor,
      [
        host, service, s = s_,
        handler = std::move(init.completion_handler)
      ]() mutable -> awaitable<void> {
        try {
          auto token = co_await this_coro::token();

          auto resolver  = tcp::resolver(s->stream.get_executor().context());
          auto endpoints = co_await resolver.async_resolve(
            asio::string_view(host.data(), host.size()),
            asio::string_view(service.data(), service.size()),
            token);

          auto endpoint = co_await asio::async_connect(
            s->stream, endpoints, token);

          co_return handler({}, endpoint);

        } catch(boost::system::error_code const& ec) {
          co_return handler(ec, tcp::endpoint());
        }
      },
      detached);

    return init.result.get();
  }

  // template <typename CompletionToken>
  // auto async_send() {

  // }
};

} // foxy

#endif // FOXY_CLIENT_SESSION_HPP_
