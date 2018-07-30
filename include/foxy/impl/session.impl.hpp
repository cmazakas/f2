#include "foxy/detail/session.hpp"
#include "foxy/detail/get_strand.hpp"

template <
  typename Serializer,
  typename WriteHeaderHandler
>
auto foxy::detail::session::async_write_header(
  Serializer&          serializer,
  WriteHeaderHandler&& write_header_handler
) & -> BOOST_ASIO_INITFN_RESULT_TYPE(
  WriteHeaderHandler, void(boost::system::error_code)
) {
  namespace beast = boost::beast;
  namespace asio  = boost::asio;
  namespace http  = boost::beast::http;

  using boost::ignore_unused;
  using boost::system::error_code;

  asio::async_completion<WriteHeaderHandler, void(boost::system::error_code)>
  init(write_header_handler);

  auto strand = foxy::detail::get_strand(
    init.completion_handler, s_->stream.get_executor());

  foxy::co_spawn(
    strand,
    [
      &serializer, s = s_,
      handler = std::move(init.completion_handler)
    ]() mutable -> foxy::awaitable<void, typename session_state::strand_type> {

      auto& stream = s->stream;

      auto executor =
        asio::get_associated_executor(handler, stream.get_executor());

      auto token       = co_await this_coro::token();
      auto ec          = error_code();
      auto error_token = redirect_error(token, ec);

      auto const bytes_transferred =
        co_await http::async_write_header(stream, serializer, error_token);

      ignore_unused(bytes_transferred);

      if (ec) {
        co_return asio::post(
          executor,
          beast::bind_handler(std::move(handler), ec));
      }

      co_return asio::post(
        executor,
        beast::bind_handler(std::move(handler), error_code()));
    },
    detached);

  return init.result.get();
}

template <
  typename Serializer,
  typename WriteHandler
>
auto foxy::detail::session::async_write(
  Serializer&    serializer,
  WriteHandler&& write_handler
) & -> BOOST_ASIO_INITFN_RESULT_TYPE(
    WriteHandler, void(boost::system::error_code)
) {
    using boost::asio::ip::tcp;
    using boost::ignore_unused;
    using boost::system::error_code;

    namespace beast = boost::beast;
    namespace asio  = boost::asio;
    namespace http  = boost::beast::http;

    asio::async_completion<WriteHandler, void(boost::system::error_code)>
    init(write_handler);

    auto strand = foxy::detail::get_strand(
      init.completion_handler, s_->stream.get_executor());

    foxy::co_spawn(
      strand,
      [
        &serializer, s = s_,
        handler = std::move(init.completion_handler)
      ]() mutable -> foxy::awaitable<void, strand_type> {

        auto token       = co_await this_coro::token();
        auto ec          = error_code();
        auto error_token = redirect_error(token, ec);

        auto executor =
          asio::get_associated_executor(handler, s->stream.get_executor());

        ignore_unused(
          co_await http::async_write(s->stream, serializer, error_token));

        if (ec) {
          co_return asio::post(
            executor,
            beast::bind_handler(std::move(handler), ec));
        }

        co_return asio::post(
          executor,
          beast::bind_handler(std::move(handler), error_code()));
      },
      detached);

    return init.result.get();
}