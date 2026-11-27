// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <array>
#include <chrono>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#include "utils/logger.h"
#include "utils/qt.h"
#include "utils/utf8_string.h"

#include <QtCore/QByteArray>
#include <QtCore/QDeadlineTimer>
#include <QtCore/QTimer>
#include <QtHttpServer/QHttpServer>
#include <QtHttpServer/QHttpServerConfiguration>
#include <QtHttpServer/QHttpServerRequest>
#include <QtHttpServer/QHttpServerResponder>
#include <QtNetwork/QAbstractSocket>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>

#include <mcpserver/protocol_engine.h>
#include <mcpserver/streamable_http_transport.h>

namespace zrythm::mcp
{

namespace
{

/** Converts the status of a reply into the status of the HTTP server. */
[[nodiscard]] auto
to_qt_status (const HttpStatus status) -> QHttpServerResponder::StatusCode
{
  return static_cast<QHttpServerResponder::StatusCode> (status);
}

[[nodiscard]] auto
to_http_method (const QHttpServerRequest::Method method) -> HttpMethod
{
  switch (method)
    {
    case QHttpServerRequest::Method::Get:
      return HttpMethod::Get;
    case QHttpServerRequest::Method::Post:
      return HttpMethod::Post;
    case QHttpServerRequest::Method::Delete:
      return HttpMethod::Delete;
    default:
      return HttpMethod::Other;
    }
}

/** The media types that a POST request must list in Accept. */
constexpr std::array<std::string_view, 2> kPostAccept{
  "application/json", "text/event-stream"
};
/** The media types that a GET request must list in Accept. */
constexpr std::array<std::string_view, 1> kGetAccept{ "text/event-stream" };

/**
 * @brief Returns the media types that the Accept header of a request with
 * @p method must list.
 *
 * The span is empty when the method needs none.
 */
[[nodiscard]] constexpr auto
required_accept (const HttpMethod method) -> std::span<const std::string_view>
{
  switch (method)
    {
    case HttpMethod::Post:
      return kPostAccept;
    case HttpMethod::Get:
      return kGetAccept;
    case HttpMethod::Delete:
    case HttpMethod::Other:
      break;
    }
  return {};
}

/** Returns true when the media type of the Content-Type header @p header is
 * application/json. */
[[nodiscard]] auto
is_json (const QByteArrayView header) -> bool
{
  QByteArrayView media_type = header;
  const auto     semicolon = media_type.indexOf (';');
  if (semicolon >= 0)
    media_type = media_type.first (semicolon);
  return media_type.toByteArray ().trimmed ().toLower () == "application/json";
}

/**
 * @brief Returns true when the Accept header lists every type in @p types.
 *
 * An item of the header counts when its media type equals the type or is a
 * wildcard. What follows a semicolon is ignored.
 */
[[nodiscard]] auto
accepts_all (
  const HttpRequest                      &request,
  const std::span<const std::string_view> types) -> bool
{
  const QByteArray accept = request.combined_header ("Accept").toLower ();
  const auto       is_listed = [accept] (const std::string_view type) {
    const auto matches = [type] (const QByteArray &item) {
      const auto semicolon = item.indexOf (';');
      const auto media_type =
        item.first (semicolon < 0 ? item.size () : semicolon).trimmed ();
      // The wildcard "type/*" covers every subtype of the type.
      const QByteArray wanted{ QByteArrayView (type) };
      const auto       type_prefix = wanted.first (wanted.indexOf ('/') + 1);
      return media_type == wanted || media_type == "*/*" || media_type == "*"
             || media_type == type_prefix + "*";
    };
    return std::ranges::any_of (accept.split (','), matches);
  };
  return std::ranges::all_of (types, is_listed);
}

/**
 * @brief Writes @p reply with @p responder.
 *
 * The responder adds the content length.
 */
auto
write_reply (QHttpServerResponder &&responder, const HttpReply &reply) -> void
{
  if (responder.isResponseCanceled ())
    return;
  QHttpHeaders headers;
  if (!reply.content_type ().isEmpty ())
    {
      headers.replaceOrAppend (
        QHttpHeaders::WellKnownHeader::ContentType, reply.content_type ());
    }
  for (const auto &header : reply.headers ())
    headers.replaceOrAppend (header.name, header.value);
  if (reply.body ().has_value ())
    {
      responder.write (
        QByteArray::fromStdString (*reply.body ()), headers,
        to_qt_status (reply.status ()));
      return;
    }
  responder.write (headers, to_qt_status (reply.status ()));
}

/** Formats @p message as one server-sent event. */
[[nodiscard]] auto
format_sse_event (const Json &message) -> QByteArray
{
  return QByteArray ("event: message\ndata: ")
         + QByteArray::fromStdString (
           message.dump (-1, ' ', false, Json::error_handler_t::replace))
         + "\n\n";
}

/** Largest time that stop() waits for the sockets to write their data. */
constexpr std::chrono::milliseconds kStopFlushBudget{ 1000 };

} // namespace

/** The exchange of one request. It owns the responder of the HTTP server. */
class HttpExchangeImpl final : public HttpExchange
{
public:
  HttpExchangeImpl (const ExchangeId id, QHttpServerResponder &&responder)
      : id_ (id), responder_ (std::move (responder))
  {
  }

  [[nodiscard]] auto id () const -> ExchangeId override { return id_; }

  [[nodiscard]] auto client_gone () const -> bool override
  {
    return responder_.has_value () && responder_->isResponseCanceled ();
  }

  [[nodiscard]] auto reply (HttpReply reply) -> bool override
  {
    if (state_ != State::Pending)
      return false;
    state_ = State::Done;
    write_reply (std::move (*responder_), reply);
    // The connection reads the next request once the responder is gone.
    responder_.reset ();
    return true;
  }

  [[nodiscard]] auto start_stream () -> bool override
  {
    if (state_ != State::Pending)
      return false;
    state_ = State::Done;
    if (client_gone ())
      {
        responder_.reset ();
        return false;
      }
    QHttpHeaders headers;
    headers.replaceOrAppend (
      QHttpHeaders::WellKnownHeader::ContentType, "text/event-stream");
    headers.replaceOrAppend (
      QHttpHeaders::WellKnownHeader::CacheControl, "no-cache");
    headers.replaceOrAppend ("X-Accel-Buffering", "no");
    responder_->writeBeginChunked (
      headers, QHttpServerResponder::StatusCode::Ok);
    state_ = State::Streaming;
    return true;
  }

  [[nodiscard]] auto send (const Json &message) -> bool override
  {
    return write_chunk (format_sse_event (message));
  }

  /** Writes a comment line that keeps an idle stream open. */
  auto send_keep_alive () -> bool { return write_chunk (":\n\n"); }

  auto close_stream () -> void override
  {
    if (state_ == State::Done)
      return;
    const bool streaming = state_ == State::Streaming;
    state_ = State::Done;
    // The last chunk must not be empty.
    if (streaming && !client_gone ())
      responder_->writeEndChunked (":\n\n");
    responder_.reset ();
  }

  /** Ends the exchange without writing anything. */
  auto abort () -> void
  {
    state_ = State::Done;
    responder_.reset ();
  }

private:
  enum class State
  {
    Pending,
    Streaming,
    Done
  };

  [[nodiscard]] auto write_chunk (const QByteArray &chunk) -> bool
  {
    if (state_ != State::Streaming || client_gone ())
      return false;
    responder_->writeChunk (chunk);
    return true;
  }

  ExchangeId                          id_;
  std::optional<QHttpServerResponder> responder_;
  State                               state_{ State::Pending };
};

StreamableHttpTransport::StreamableHttpTransport (
  const QString                 endpoint_path,
  const ServerLimits            limits,
  ProtocolEngine::Authenticator authenticator,
  McpRequestListener           &listener,
  QObject *                     parent)
    : QObject (parent), endpoint_path_ (endpoint_path), limits_ (limits),
      authenticator_ (std::move (authenticator)), listener_ (listener)
{
  // The HTTP server takes the ownership of the TCP server when it binds
  // to it.
  tcp_server_ = utils::make_qobject_unique<QTcpServer> (this);
  http_server_ = std::make_unique<QHttpServer> ();
  QHttpServerConfiguration configuration;
  // The HTTP server refuses a body over the limit before it reads the body,
  // and a connection over the limit as soon as it accepts the connection.
  configuration.setMaximumBodySize (
    static_cast<qint64> (limits_.max_request_body_bytes));
  configuration.setMaximumConnections (limits_.max_connections);
  http_server_->setConfiguration (configuration);
  http_server_->route (
    endpoint_path_, QHttpServerRequest::Method::AnyKnown, this,
    [this] (const QHttpServerRequest &request, QHttpServerResponder &&responder) {
      handle (request, std::move (responder));
    });
  http_server_->setMissingHandler (
    this,
    [this] (const QHttpServerRequest &request, QHttpServerResponder &responder) {
      handle_unrouted (request, responder);
    });
  liveness_timer_ = utils::make_qobject_unique<QTimer> (this);
  liveness_timer_->setInterval (limits_.stream_liveness_interval);
  QObject::connect (
    liveness_timer_.get (), &QTimer::timeout, this,
    &StreamableHttpTransport::check_client_liveness);
  keep_alive_timer_ = utils::make_qobject_unique<QTimer> (this);
  keep_alive_timer_->setInterval (limits_.stream_keep_alive_interval);
  QObject::connect (
    keep_alive_timer_.get (), &QTimer::timeout, this,
    &StreamableHttpTransport::send_keep_alives);
}

StreamableHttpTransport::~StreamableHttpTransport ()
{
  stop ();
}

auto
StreamableHttpTransport::listen (const QHostAddress &address, const quint16 port)
  -> std::expected<void, ListenError>
{
  if (tcp_server_ == nullptr)
    {
      return std::unexpected (
        ListenError ("The transport stopped, so it cannot serve again"));
    }
  if (tcp_server_->isListening ())
    return std::unexpected (ListenError ("The transport already serves"));
  if (!tcp_server_->listen (address, port))
    {
      return std::unexpected (ListenError (
        tcp_server_->serverError (),
        std::string (
          utils::Utf8String::from_qstring (tcp_server_->errorString ()).view ())));
    }
  if (!http_server_->bind (tcp_server_.get ()))
    {
      return std::unexpected (
        ListenError ("The HTTP server could not bind to the TCP server"));
    }
  liveness_timer_->start ();
  keep_alive_timer_->start ();
  return {};
}

auto
StreamableHttpTransport::stop () -> void
{
  liveness_timer_->stop ();
  keep_alive_timer_->stop ();
  if (http_server_ == nullptr)
    return;
  // Every responder must be gone before the HTTP server is destroyed.
  std::vector<std::shared_ptr<HttpExchangeImpl>> open;
  for (const auto &weak : exchanges_)
    {
      if (auto exchange = weak.lock ())
        open.push_back (std::move (exchange));
    }
  for (const auto &exchange : open)
    exchange->abort ();
  exchanges_.clear ();
  // The HTTP server parents every accepted socket to the protocol handler
  // of its connection, so the sockets are found under the HTTP server. The
  // last events of the streams are in the write buffers of the sockets. The
  // sockets write them and close gracefully, because a destroyed socket
  // resets the connection and drops what its client did not read yet.
  const auto     sockets = http_server_->findChildren<QTcpSocket *> ();
  QDeadlineTimer deadline (kStopFlushBudget);
  for (auto * socket : sockets)
    {
      socket->disconnectFromHost ();
    }
  for (auto * socket : sockets)
    {
      while (
        socket->state () != QAbstractSocket::UnconnectedState
        && !deadline.hasExpired ())
        {
          socket->waitForDisconnected (deadline.remainingTime ());
        }
    }
  // The destruction of the HTTP server closes the listening socket and
  // destroys the TCP server, which it owns.
  http_server_.reset ();
}

auto
StreamableHttpTransport::is_listening () const -> bool
{
  if (tcp_server_ == nullptr)
    return false;
  return tcp_server_->isListening ();
}

auto
StreamableHttpTransport::listening_port () const -> quint16
{
  return tcp_server_ == nullptr ? 0 : tcp_server_->serverPort ();
}

auto
StreamableHttpTransport::handle_unrouted (
  const QHttpServerRequest &request,
  QHttpServerResponder     &responder) -> void
{
  const auto reply =
    request.url ().path () == endpoint_path_
      ? HttpReply::method_not_allowed (
          { HttpMethod::Post, HttpMethod::Get, HttpMethod::Delete })
      : HttpReply::without_body (HttpStatus::NotFound);
  write_reply (std::move (responder), reply);
}

auto
StreamableHttpTransport::authenticate (const HttpRequest &request) const
  -> std::expected<ProtocolEngine::Identity, HttpReply>
{
  try
    {
      auto identity = authenticator_ (request);
      // An empty identity means that the authenticator refused the client.
      if (!identity.has_value ())
        {
          return std::unexpected (
            HttpReply::without_body (
              HttpStatus::Unauthorized,
              {
                { "WWW-Authenticate", "Bearer realm=\"mcp\"" }
          }));
        }
      return identity;
    }
  catch (const std::exception &exception)
    {
      z_warning ("The authenticator threw an exception: {}", exception.what ());
    }
  catch (...)
    {
      z_warning ("The authenticator threw an exception");
    }
  return std::unexpected (
    HttpReply::without_body (HttpStatus::InternalServerError));
}

auto
StreamableHttpTransport::handle (
  const QHttpServerRequest &request,
  QHttpServerResponder    &&responder) -> void
{
  const auto refuse = [&responder] (HttpReply reply) {
    write_reply (std::move (responder), std::move (reply));
  };
  // The server talks to clients that hold a token, so a browser is refused.
  if (request.headers ().contains ("Origin"))
    {
      refuse (HttpReply::without_body (HttpStatus::Forbidden));
      return;
    }
  if (request.url ().path () != endpoint_path_)
    {
      refuse (HttpReply::without_body (HttpStatus::NotFound));
      return;
    }
  const auto method = to_http_method (request.method ());
  if (method == HttpMethod::Other)
    {
      refuse (
        HttpReply::method_not_allowed (
          { HttpMethod::Post, HttpMethod::Get, HttpMethod::Delete }));
      return;
    }
  const HttpRequest http_request (
    method, request.url (), request.headers (), request.body ());
  auto identity = authenticate (http_request);
  if (!identity.has_value ())
    {
      refuse (std::move (identity).error ());
      return;
    }
  if (!accepts_all (http_request, required_accept (method)))
    {
      refuse (HttpReply::without_body (HttpStatus::NotAcceptable));
      return;
    }
  if (
    method == HttpMethod::Post
    && !is_json (http_request.header ("Content-Type")))
    {
      refuse (HttpReply::without_body (HttpStatus::UnsupportedMediaType));
      return;
    }

  auto exchange = std::make_shared<HttpExchangeImpl> (
    next_exchange_id_++, std::move (responder));
  exchanges_.emplace_back (exchange);
  switch (method)
    {
    case HttpMethod::Post:
      {
        // The JSON parser validates the UTF-8 of the body.
        const auto &body = http_request.body ();
        listener_.on_post (
          http_request,
          parse_json (
            std::string_view (
              body.constData (), static_cast<std::size_t> (body.size ()))),
          identity.value (), std::move (exchange));
        break;
      }
    case HttpMethod::Get:
      listener_.on_get (http_request, std::move (exchange));
      break;
    case HttpMethod::Delete:
      listener_.on_delete (http_request, std::move (exchange));
      break;
    case HttpMethod::Other:
      z_return_if_reached ();
    }
}

auto
StreamableHttpTransport::check_client_liveness () -> void
{
  std::erase_if (exchanges_, [] (const auto &weak) { return weak.expired (); });
  for (const auto &weak : exchanges_)
    {
      const auto exchange = weak.lock ();
      if (exchange != nullptr && exchange->client_gone ())
        listener_.on_client_gone (exchange->id ());
    }
}

auto
StreamableHttpTransport::send_keep_alives () -> void
{
  for (const auto &weak : exchanges_)
    {
      if (const auto exchange = weak.lock (); exchange != nullptr)
        exchange->send_keep_alive ();
    }
}

} // namespace zrythm::mcp
