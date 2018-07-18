#ifndef FOXY_SERVER_SESSION_HPP_
#define FOXY_SERVER_SESSION_HPP_

#include <boost/asio/post.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/async_result.hpp>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>

#include <boost/asio/strand.hpp>
#include <boost/asio/executor.hpp>
#include <boost/asio/associated_executor.hpp>
#include <boost/asio/associated_allocator.hpp>

#include <boost/system/error_code.hpp>

#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>

#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/bind_handler.hpp>

#include <boost/core/ignore_unused.hpp>

#include <memory>
#include <utility>
#include <iostream>
#include <string_view>

#include "foxy/coroutine.hpp"
#include "foxy/multi_stream.hpp"

namespace foxy {

struct server_session {

public:
  using timer_type  = boost::asio::steady_timer;
  using buffer_type = boost::beast::flat_buffer;
  using stream_type = multi_stream;
  using strand_type = boost::asio::strand<boost::asio::executor>;

private:
  struct session_state {
    stream_type stream;
    strand_type strand;
    timer_type  timer;
    buffer_type buffer;

    session_state()                     = delete;
    session_state(session_state const&) = default;
    session_state(session_state&&)      = default;

    explicit
    session_state(stream_type stream_);
  };

  std::shared_ptr<session_state> s_;

public:
  server_session()                      = delete;
  server_session(server_session const&) = default;
  server_session(server_session&&)      = default;

  explicit
  server_session(multi_stream stream);

  auto shutdown() -> void;

  template <
    typename Parser,
    typename ReadHandler
  >
  auto async_read(
    Parser&       parser,
    ReadHandler&& read_handler
  ) & -> BOOST_ASIO_INITFN_RESULT_TYPE(
    ReadHandler, void(boost::system::error_code)
  ) {

    namespace beast = boost::beast;
    namespace asio  = boost::asio;
    namespace http  = beast::http;
    using boost::system::error_code;
    using boost::ignore_unused;

    asio::async_completion<ReadHandler, void(boost::system::error_code)>
    init(read_handler);

    co_spawn(
      s_->strand,
      [
        &parser,
        s       = s_,
        handler = std::move(init.completion_handler)
      ]() mutable -> awaitable<void, strand_type> {

        auto executor =
          asio::get_associated_executor(handler, s->stream.get_executor());

        auto token       = co_await this_coro::token();
        auto ec          = error_code();
        auto error_token = redirect_error(token, ec);

        ignore_unused(
          co_await http::async_read(
            s->stream,
            s->buffer,
            parser,
            error_token));

        if (ec) {
          co_return asio::post(
            executor, beast::bind_handler(std::move(handler), ec));
        }

        co_return asio::post(
          executor,
          beast::bind_handler(std::move(handler), error_code()));
      },
      detached);

    return init.result.get();
  }

  template <
    typename Message,
    typename WriteHandler
  >
  auto async_write(
    Message&       message,
    WriteHandler&& write_handler
  ) & -> BOOST_ASIO_INITFN_RESULT_TYPE(
    WriteHandler, void(boost::system::error_code)
  ) {

    namespace beast = boost::beast;
    namespace asio  = boost::asio;
    namespace http  = beast::http;
    using boost::system::error_code;
    using boost::ignore_unused;

    asio::async_completion<WriteHandler, void(boost::system::error_code)>
    init(write_handler);

    co_spawn(
      s_->strand,
      [
        &message,
        s       = s_,
        handler = std::move(init.completion_handler)
      ]() mutable -> awaitable<void, strand_type> {

        auto executor =
          asio::get_associated_executor(handler, s->stream.get_executor());

        auto token       = co_await this_coro::token();
        auto ec          = error_code();
        auto error_token = redirect_error(token, ec);

        ignore_unused(
          co_await http::async_write(s->stream, message, error_token));

        if (ec) {
          co_return asio::post(
            executor, beast::bind_handler(std::move(handler), ec));
        }

        co_return asio::post(
          executor, beast::bind_handler(std::move(handler), error_code()));
      },
      detached);

    return init.result.get();
  }
};

} // foxy

#endif // FOXY_SERVER_SESSION_HPP_