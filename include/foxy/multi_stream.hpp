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
#include <boost/asio/associated_executor.hpp>

namespace foxy {

struct multi_stream {

public:
  using stream_type     = boost::asio::ip::tcp::socket;
  using ssl_stream_type = boost::asio::ssl::stream<boost::asio::ip::tcp::socket&>;
  using executor_type   = boost::asio::ip::tcp::socket::executor_type;

private:
  stream_type                    stream_;
  std::optional<ssl_stream_type> ssl_stream_;

public:
  multi_stream()                    = delete;
  multi_stream(multi_stream const&) = default;
  multi_stream(multi_stream&&)      = default;

  explicit
  multi_stream(boost::asio::io_context& io);

  explicit
  multi_stream(boost::asio::io_context& io, boost::asio::ssl::context& ctx);

  auto get_executor() -> executor_type;

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

  auto encrypted() const -> bool;
  auto next_layer() &    -> stream_type&;
  auto stream() &        -> stream_type&;
  auto ssl_stream() &    -> ssl_stream_type&;
};

} // foxy

#endif // FOXY_MULTI_STREAM_HPP_
