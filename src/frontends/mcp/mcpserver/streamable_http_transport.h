// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <expected>
#include <memory>
#include <vector>

#include "utils/qt.h"

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpServer>

#include <mcpserver/http_exchange.h>
#include <mcpserver/protocol_engine.h>

class QHttpServer;
class QHttpServerRequest;
class QHttpServerResponder;

namespace zrythm::mcp
{

/** The server implementation that answers the requests of the endpoint. */
class HttpExchangeImpl;

/** The receiver of the requests of the endpoint. */
class McpRequestListener
{
public:
  virtual ~McpRequestListener () = default;

  /**
   * @brief A POST request.
   *
   * @p body holds the parsed JSON, or the error response for a body that is
   * not valid JSON.
   */
  virtual auto on_post (
    const HttpRequest                        &request,
    std::expected<Json, JsonRpcErrorResponse> body,
    const ProtocolEngine::Identity           &identity,
    std::shared_ptr<HttpExchange>             exchange) -> void = 0;

  virtual auto
  on_get (const HttpRequest &request, std::shared_ptr<HttpExchange> exchange)
    -> void = 0;

  virtual auto
  on_delete (const HttpRequest &request, std::shared_ptr<HttpExchange> exchange)
    -> void = 0;

  /** The client of the open exchange @p id closed its connection. */
  virtual auto on_client_gone (ExchangeId id) -> void = 0;
};

/**
 * @brief Serves one path over HTTP and reports each request to a listener.
 *
 * The transport refuses the requests that it does not pass to the listener: a
 * request with an Origin header gets 403, a request of a refused client gets
 * 401, a request without the needed media types in Accept gets 406, and a POST
 * request without the Content-Type application/json gets 415. Qt HTTP Server
 * refuses a request before the transport sees it in two cases: a body over
 * max_request_body_bytes gets 413, and a connection over max_connections gets
 * 429. All functions must be called on the thread that owns this object.
 *
 * The transport serves once: it cannot listen again after stop().
 */
class StreamableHttpTransport final : public QObject
{
  Q_OBJECT

public:
  StreamableHttpTransport (
    QString                       endpoint_path,
    ServerLimits                  limits,
    ProtocolEngine::Authenticator authenticator,
    McpRequestListener           &listener,
    QObject *                     parent = nullptr);
  ~StreamableHttpTransport () override;
  StreamableHttpTransport (const StreamableHttpTransport &) = delete;
  StreamableHttpTransport &operator= (const StreamableHttpTransport &) = delete;

  /** Serves on @p address and @p port. The port may be 0 to choose a free port.
   */
  [[nodiscard]] auto listen (const QHostAddress &address, quint16 port)
    -> std::expected<void, ListenError>;

  /** Ends every open exchange and stops serving for good. */
  auto stop () -> void;

  [[nodiscard]] auto is_listening () const -> bool;

  /** The port that the transport serves on. It is 0 while the transport does
   * not serve. */
  [[nodiscard]] auto listening_port () const -> quint16;

private:
  auto
  handle (const QHttpServerRequest &request, QHttpServerResponder &&responder)
    -> void;
  auto handle_unrouted (
    const QHttpServerRequest &request,
    QHttpServerResponder     &responder) -> void;
  /**
   * @brief Identifies the client of @p request.
   *
   * The error is the reply that refuses the request: 401 when the
   * authenticator refuses the client, 500 when the authenticator throws.
   */
  [[nodiscard]] auto authenticate (const HttpRequest &request) const
    -> std::expected<ProtocolEngine::Identity, HttpReply>;
  auto check_client_liveness () -> void;
  auto send_keep_alives () -> void;

  QString                       endpoint_path_;
  ServerLimits                  limits_;
  ProtocolEngine::Authenticator authenticator_;
  McpRequestListener           &listener_;
  /** The TCP server. It is a child of the transport until the HTTP server
   * binds to it and takes its ownership, so the pointer is null once the HTTP
   * server is gone. */
  utils::QObjectUniquePtr<QTcpServer>          tcp_server_;
  std::unique_ptr<QHttpServer>                 http_server_;
  utils::QObjectUniquePtr<QTimer>              liveness_timer_;
  utils::QObjectUniquePtr<QTimer>              keep_alive_timer_;
  ExchangeId                                   next_exchange_id_{ 1 };
  std::vector<std::weak_ptr<HttpExchangeImpl>> exchanges_;
};

} // namespace zrythm::mcp
