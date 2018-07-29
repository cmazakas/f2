#ifndef FOXY_DETAIL_SESSION_HPP_
#define FOXY_DETAIL_SESSION_HPP_

#include "foxy/coroutine.hpp"
#include "foxy/detail/session_state.hpp"
#include <memory>

namespace foxy {
namespace detail {

struct session {
protected:
  std::shared_ptr<session_state> s_;

public:
  using timer_type  = session_state::timer_type;
  using buffer_type = session_state::buffer_type;
  using stream_type = session_state::stream_type;
  using strand_type = session_state::strand_type;

  // client sessions cannot be default-constructed as they require an
  // `io_context`
  //
  session()               = delete;

  session(session const&) = default;
  session(session&&)      = default;

  explicit
  session(boost::asio::io_context& io);

  // when constructed with an SSL context, the `session` will use the SSL side
  // of the `foxy::multi_stream`
  // sessions constructed with an SSL context need to be shutdown using
  // `async_ssl_shutdown`
  //
  explicit
  session(boost::asio::io_context& io, boost::asio::ssl::context& ctx);

  // `async_write_header` mirrors the Beast function `http::async_write_header`
  // and will write the header portion of the Serializer to the session's
  // underlying stream object
  //
  template <
    typename Serializer,
    typename WriteHeaderHandler
  >
  auto async_write_header(
    Serializer&          serializer,
    WriteHeaderHandler&& write_header_handler
  ) & -> BOOST_ASIO_INITFN_RESULT_TYPE(
    WriteHeaderHandler, void(boost::system::error_code));

  // `async_write` mirrors the Beast function, `http::async_write` and writes
  // the input Serializer through the `client_session` to the currently
  // connected remote host
  //
  template <
    typename Serializer,
    typename WriteHandler
  >
  auto async_write(
    Serializer&    serializer,
    WriteHandler&& write_handler
  ) & -> BOOST_ASIO_INITFN_RESULT_TYPE(
      WriteHandler, void(boost::system::error_code));
};

} // detail
} // foxy

#include "foxy/impl/session.impl.hpp"

#endif // FOXY_DETAIL_SESSION_HPP_