#ifndef FOXY_MULTI_STREAM_HPP_
#define FOXY_MULTI_STREAM_HPP_

#include <utility>
#include <optional>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/system/error_code.hpp>

namespace foxy {

struct multi_stream {
public:
  using stream_type     = boost::asio::ip::tcp::socket;
  using ssl_stream_type = boost::asio::ssl::stream<stream_type&>;

  stream_type                    stream_;
  std::optional<ssl_stream_type> ssl_stream_;

  multi_stream(boost::asio::io_context& io)
  : stream_(io)
  {
  }

  auto get_executor() {
    return stream_.get_executor();
  }

  template <
    typename MutableBufferSequence,
    typename ReadHandler
  >
  auto async_read_some(
    MutableBufferSequence const& buffers,
    ReadHandler&&                handler
  ) {
    return ssl_stream_
      ? ssl_stream_.value().async_read_some(
        buffers, std::forward<ReadHandler>(handler))
      : stream_.async_read_some(buffers, std::forward<ReadHandler>(handler));
  }

  template<
    typename ConstBufferSequence,
    typename WriteHandler
  >
  auto async_write_some(
    ConstBufferSequence const& buffers,
    WriteHandler&&             handler
  ) {
    return ssl_stream_
      ? ssl_stream_.value().async_write_some(
        buffers, std::forward<WriteHandler>(handler))
      : stream_.async_write_some(buffers, std::forward<WriteHandler>(handler));
  }

  auto encrypted() const -> bool {
    return static_cast<bool>(ssl_stream_);
  }

  auto next_layer() & -> stream_type& {
    return ssl_stream_ ? ssl_stream_->next_layer() : stream_;
  }


  template <typename CompletionToken>
  auto async_shutdown(CompletionToken&& token) {

    boost::asio::async_completion<
      CompletionToken,
      void(boost::system::error_code)
    > init(token);

    co_spawn(
      this->get_executor(),
      [&, handler = std::move(init.completion_handler)]
      () mutable -> awaitable<void> {

        auto token = co_await this_coro::token();
        if (ssl_stream_) {
          (void ) co_await ssl_stream_->async_shutdown(token);
        } else {
          stream_.shutdown(boost::asio::ip::tcp::socket::shutdown_send);
        }

        co_return handler({});
      },
      detached);

    return init.result.get();
  }
};

} // foxy

#endif // FOXY_MULTI_STREAM_HPP_
