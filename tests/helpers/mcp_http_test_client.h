// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "utils/utf8_string.h"

#include <QAbstractSocket>
#include <QByteArray>
#include <QDebug>
#include <QEventLoop>
#include <QHostAddress>
#include <QList>
#include <QMap>
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

namespace zrythm::mcp::test
{

/** How long a client waits for the server by default. */
inline constexpr std::chrono::milliseconds kClientTimeout{ 10000 };

/** The name of the stateless protocol revision. */
inline constexpr std::string_view kStatelessRevision{ "2026-07-28" };

inline auto
to_bytes (const std::string_view text) -> QByteArray
{
  return QByteArray (text.data (), static_cast<qsizetype> (text.size ()));
}

/**
 * A JSON-RPC request.
 *
 * @p params is sent as given. No params member is sent when it is null.
 */
inline auto
rpc_request (
  int                    id,
  const std::string_view method,
  nlohmann::json         params = nlohmann::json ()) -> nlohmann::json
{
  nlohmann::json message = nlohmann::json::object ();
  message["jsonrpc"] = "2.0";
  message["id"] = id;
  message["method"] = std::string (method);
  if (!params.is_null ())
    {
      message["params"] = std::move (params);
    }
  return message;
}

/** A JSON-RPC notification. */
inline auto
rpc_notification (
  const std::string_view method,
  nlohmann::json         params = nlohmann::json ()) -> nlohmann::json
{
  auto message = rpc_request (0, method, std::move (params));
  message.erase ("id");
  return message;
}

/**
 * The params of a request in the stateless revision.
 *
 * The server requires the protocol revision and the capabilities of the
 * client in the _meta of the params.
 */
inline auto
stateless_params (nlohmann::json params) -> nlohmann::json
{
  params["_meta"] = {
    { "io.modelcontextprotocol/protocolVersion",
     std::string (kStatelessRevision)                                         },
    { "io.modelcontextprotocol/clientCapabilities", nlohmann::json::object () }
  };
  return params;
}

/**
 * A minimal MCP-over-HTTP client that speaks raw TCP.
 *
 * A request opens its own connection, and a request whose response is a
 * stream keeps that connection open. Waits run the event loop, so the
 * code of the thread under test runs while a response is pending.
 */
class McpHttpClient
{
public:
  struct Header
  {
    QByteArray name;
    QByteArray value;
  };

  /** One HTTP request. */
  struct Request
  {
    QByteArray          method{ "POST" };
    QByteArray          path{ "/mcp" };
    std::vector<Header> headers{};
    QByteArray          body{};

    /** Sets the body to @p message and adds the headers of a JSON request. */
    auto json_body (const nlohmann::json &message) -> Request &
    {
      body = QByteArray::fromStdString (message.dump ());
      header ("Content-Type", "application/json");
      header ("Accept", "application/json, text/event-stream");
      return *this;
    }

    /** Sets the body to @p bytes and adds no header. */
    auto raw_body (QByteArray bytes) -> Request &
    {
      body = std::move (bytes);
      return *this;
    }

    auto header (const QByteArray &name, const QByteArray &value) -> Request &
    {
      headers.push_back ({ name, value });
      return *this;
    }

    /** Sets the Accept header to @p value. */
    auto accept (const QByteArray &value) -> Request &
    {
      return header ("Accept", value);
    }

    /** Adds the authorization of the bearer @p token. */
    auto bearer (const QByteArray &token) -> Request &
    {
      return header ("Authorization", "Bearer " + token);
    }

    /**
     * Adds the headers that name a request of the stateless revision.
     *
     * The Mcp-Name header is added when @p name is not empty.
     */
    auto stateless (
      const std::string_view method_name,
      const std::string_view name = {},
      const std::string_view version = kStatelessRevision) -> Request &
    {
      header ("MCP-Protocol-Version", to_bytes (version));
      header ("Mcp-Method", to_bytes (method_name));
      if (!name.empty ())
        {
          header ("Mcp-Name", to_bytes (name));
        }
      return *this;
    }

    /** Adds the session header of the legacy revision. */
    auto session (const QByteArray &id) -> Request &
    {
      return header ("Mcp-Session-Id", id);
    }
  };

  /** The status and the headers of a response, with its body. */
  struct Response
  {
    int                          status = 0;
    QMap<QByteArray, QByteArray> headers;
    /** The header block as the server sent it. */
    QByteArray raw_headers;
    QByteArray body;

    /** The body as JSON. The value is discarded when the body is not JSON. */
    [[nodiscard]] auto json () const -> nlohmann::json
    {
      return nlohmann::json::parse (body, nullptr, false);
    }

    /** The value of the header @p name, compared without case, or an empty
     * array. */
    [[nodiscard]] auto header (const QByteArray &name) const -> QByteArray
    {
      return headers.value (name.toLower ());
    }

    /** True when the Allow header lists @p method. */
    [[nodiscard]] auto allows (const QByteArray &method) const -> bool
    {
      return std::ranges::any_of (
        header ("Allow").split (','), [&method] (const QByteArray &allowed) {
          return allowed.trimmed () == method;
        });
    }
  };

  /** One event of a stream. */
  struct Event
  {
    /** The value of the event field, when the event has one. */
    std::optional<QByteArray> name;
    /** The data lines of the event, joined by newlines. */
    QByteArray data;

    /** The data as JSON. The value is discarded when the data is not JSON. */
    [[nodiscard]] auto json () const -> nlohmann::json
    {
      return nlohmann::json::parse (data, nullptr, false);
    }
  };

  /** The response to a request that stays open. */
  class Stream
  {
  public:
    Stream (const Stream &) = delete;
    auto operator= (const Stream &) -> Stream & = delete;

    ~Stream () { close (); }

    /** The status and the headers of the response. */
    [[nodiscard]] auto response () const -> const Response &
    {
      return response_;
    }

    /**
     * @brief Waits for the next event.
     *
     * Comment lines are skipped. Returns no value when the server ended
     * the stream or when @p timeout passed.
     */
    auto next_event (std::chrono::milliseconds timeout = kClientTimeout)
      -> std::optional<Event>;

    /** True once the server ended the stream or the connection. */
    [[nodiscard]] auto ended () const -> bool { return ended_; }

    /** Ends the stream from the side of the client. */
    auto close () -> void;

  private:
    friend class McpHttpClient;
    Stream () = default;

    /** Moves the bytes that arrived into the events. */
    auto decode () -> void;

    QTcpSocket *          socket_{}; // NOLINT(*-use-default-member-initializer)
    Response              response_;
    QByteArray            wire_;
    QByteArray            payload_;
    bool                  chunked_{ false };
    std::optional<qint64> chunk_bytes_left_{};
    bool                  ended_{ false };
  };

  explicit McpHttpClient (quint16 port) : port_ (port) { }

  /** The response to a request that has been sent and not read yet. */
  class PendingResponse
  {
  public:
    PendingResponse (const PendingResponse &) = delete;
    auto operator= (const PendingResponse &) -> PendingResponse & = delete;

    /** Reads the response. The status is 0 when no response arrives. */
    [[nodiscard]] auto wait () -> Response
    {
      Response response;
      if (!read_headers (socket_, response))
        {
          ADD_FAILURE () << "no HTTP response arrived";
          return response;
        }
      read_body (socket_, response);
      return response;
    }

  private:
    friend class McpHttpClient;
    PendingResponse () = default;

    QTcpSocket socket_;
  };

  /** Sends @p request on a new connection and reads the whole response. */
  [[nodiscard]] auto send (const Request &request) -> Response
  {
    return start_request (request)->wait ();
  }

  /**
   * @brief Sends @p request on a new connection without reading its
   * response.
   *
   * The server handles the request while the event loop runs. The
   * response is read by wait() of the returned object.
   */
  [[nodiscard]] auto
  start_request (const Request &request) -> std::unique_ptr<PendingResponse>
  {
    auto pending = std::unique_ptr<PendingResponse> (new PendingResponse ());
    if (!connect (pending->socket_, port_))
      {
        return pending;
      }
    write_request (pending->socket_, request, true);
    return pending;
  }

  /**
   * @brief Sends @p requests one after another on one connection.
   *
   * Each request is sent after the response to the previous one arrived.
   * The responses are returned in the order of the requests. The list is
   * shorter when a response does not arrive.
   */
  [[nodiscard]] auto
  send_in_order (const std::vector<Request> &requests) -> std::vector<Response>;

  /**
   * @brief Sends @p request on a new connection and returns its response
   * as a stream, once the headers arrived.
   */
  [[nodiscard]] auto
  open_stream (const Request &request) -> std::unique_ptr<Stream>;

  /** A TCP port that no one listens on. */
  [[nodiscard]] static auto pick_free_port () -> quint16
  {
    QTcpServer probe;
    probe.listen (QHostAddress::LocalHost, 0);
    const auto port = probe.serverPort ();
    probe.close ();
    return port;
  }

  /** A port that no server listens on. */
  static constexpr quint16 kNoServerPort = 1;

private:
  /** Runs the event loop until @p ready holds. Returns false when @p timeout
   * passed. */
  template <typename Predicate>
  static auto wait_until (
    QTcpSocket               &socket,
    Predicate                 ready,
    std::chrono::milliseconds timeout = kClientTimeout) -> bool
  {
    QTimer deadline;
    deadline.setSingleShot (true);
    deadline.start (timeout);
    while (!ready ())
      {
        if (!deadline.isActive ())
          {
            return false;
          }
        QEventLoop loop;
        QObject::connect (
          &socket, &QTcpSocket::readyRead, &loop, &QEventLoop::quit);
        QObject::connect (
          &socket, &QTcpSocket::disconnected, &loop, &QEventLoop::quit);
        QObject::connect (&deadline, &QTimer::timeout, &loop, &QEventLoop::quit);
        loop.exec ();
      }
    return true;
  }

  static auto connect (QTcpSocket &socket, quint16 port) -> bool;
  static auto write_request (
    QTcpSocket    &socket,
    const Request &request,
    bool           close_after_response) -> void;
  static auto read_headers (QTcpSocket &socket, Response &response) -> bool;
  static auto read_body (QTcpSocket &socket, Response &response) -> void;
  static auto read_line (QTcpSocket &socket) -> QByteArray;

  quint16 port_;
};

inline auto
McpHttpClient::Stream::next_event (const std::chrono::milliseconds timeout)
  -> std::optional<Event>
{
  QTimer deadline;
  deadline.setSingleShot (true);
  deadline.start (timeout);
  for (;;)
    {
      wire_ += socket_->readAll ();
      decode ();
      for (;;)
        {
          const auto end = payload_.indexOf ("\n\n");
          if (end < 0)
            {
              break;
            }
          Event             event;
          const auto        lines = payload_.left (end).split ('\n');
          QList<QByteArray> data_lines;
          for (const auto &line : lines)
            {
              if (line.startsWith ("data:"))
                {
                  data_lines.push_back (line.mid (5).trimmed ());
                }
              else if (line.startsWith ("event:"))
                {
                  event.name = line.mid (6).trimmed ();
                }
            }
          payload_.remove (0, end + 2);
          if (!data_lines.empty ())
            {
              event.data = data_lines.join ('\n');
              return event;
            }
        }
      if (
        ended_ || socket_->state () == QAbstractSocket::UnconnectedState
        || !deadline.isActive ())
        {
          return std::nullopt;
        }
      QEventLoop loop;
      QObject::connect (
        socket_, &QTcpSocket::readyRead, &loop, &QEventLoop::quit);
      QObject::connect (
        socket_, &QTcpSocket::disconnected, &loop, &QEventLoop::quit);
      QObject::connect (&deadline, &QTimer::timeout, &loop, &QEventLoop::quit);
      loop.exec ();
    }
}

inline void
McpHttpClient::Stream::decode ()
{
  if (!chunked_)
    {
      payload_ += wire_;
      wire_.clear ();
      payload_.replace ("\r\n", "\n");
      return;
    }
  for (;;)
    {
      if (!chunk_bytes_left_.has_value ())
        {
          const auto line_end = wire_.indexOf ('\n');
          if (line_end < 0)
            {
              return;
            }
          const auto size_line = wire_.left (line_end).trimmed ();
          wire_.remove (0, line_end + 1);
          const auto digits = size_line.left (
            size_line.indexOf (';') < 0
              ? size_line.size ()
              : size_line.indexOf (';'));
          bool       ok = false;
          const auto size = digits.toLong (&ok, 16);
          if (!ok || size == 0)
            {
              // The last chunk ends the body.
              ended_ = true;
              return;
            }
          chunk_bytes_left_ = size;
        }
      const auto size = *chunk_bytes_left_;
      if (wire_.size () < size + 2)
        {
          return;
        }
      payload_ += wire_.left (size);
      // Each chunk ends with a CRLF.
      wire_.remove (0, size + 2);
      chunk_bytes_left_.reset ();
    }
}

inline void
McpHttpClient::Stream::close ()
{
  if (socket_ == nullptr)
    {
      return;
    }
  socket_->disconnectFromHost ();
  if (socket_->state () != QAbstractSocket::UnconnectedState)
    {
      socket_->waitForDisconnected (1000);
    }
  delete std::exchange (socket_, nullptr);
}

inline auto
McpHttpClient::send_in_order (const std::vector<Request> &requests)
  -> std::vector<Response>
{
  std::vector<Response> responses;
  QTcpSocket            socket;
  if (requests.empty () || !connect (socket, port_))
    {
      return responses;
    }
  for (std::size_t index = 0; index < requests.size (); ++index)
    {
      const auto last = index + 1 == requests.size ();
      write_request (socket, requests[index], last);
      Response response;
      if (!read_headers (socket, response))
        {
          ADD_FAILURE () << "no HTTP response arrived for request " << index;
          return responses;
        }
      read_body (socket, response);
      responses.push_back (std::move (response));
    }
  return responses;
}

inline auto
McpHttpClient::open_stream (const Request &request) -> std::unique_ptr<Stream>
{
  auto stream = std::unique_ptr<Stream> (new Stream ());
  stream->socket_ = new QTcpSocket ();
  if (!connect (*stream->socket_, port_))
    {
      return stream;
    }
  write_request (*stream->socket_, request, false);
  if (!read_headers (*stream->socket_, stream->response_))
    {
      ADD_FAILURE () << "no HTTP response arrived for the stream request";
      return stream;
    }
  stream->chunked_ =
    stream->response_.header ("Transfer-Encoding").toLower ().contains ("chunked");
  stream->wire_ = stream->socket_->readAll ();
  stream->decode ();
  return stream;
}

inline auto
McpHttpClient::connect (QTcpSocket &socket, const quint16 port) -> bool
{
  socket.connectToHost (QHostAddress::LocalHost, port);
  if (!socket.waitForConnected (5000))
    {
      ADD_FAILURE ()
        << "connect failed:"
        << utils::Utf8String::from_qstring (socket.errorString ()).view ();
      return false;
    }
  return true;
}

inline void
McpHttpClient::write_request (
  QTcpSocket    &socket,
  const Request &request,
  const bool     close_after_response)
{
  QByteArray wire;
  wire += request.method + " " + request.path + " HTTP/1.1\r\n";
  wire += "Host: 127.0.0.1\r\n";
  for (const auto &header : request.headers)
    {
      wire += header.name + ": " + header.value + "\r\n";
    }
  wire +=
    close_after_response ? "Connection: close\r\n" : "Connection: keep-alive\r\n";
  wire +=
    "Content-Length: " + QByteArray::number (request.body.size ()) + "\r\n\r\n";
  wire += request.body;
  socket.write (wire);
  while (socket.bytesToWrite () > 0)
    {
      socket.waitForBytesWritten (5000);
    }
}

inline auto
McpHttpClient::read_line (QTcpSocket &socket) -> QByteArray
{
  const auto ready = wait_until (socket, [&socket] {
    return socket.canReadLine ();
  });
  return ready ? socket.readLine () : QByteArray ();
}

inline auto
McpHttpClient::read_headers (QTcpSocket &socket, Response &response) -> bool
{
  const auto status_line = read_line (socket);
  if (status_line.isEmpty ())
    {
      return false;
    }
  const auto parts = status_line.simplified ().split (' ');
  if (parts.size () >= 2)
    {
      response.status = parts.at (1).toInt ();
    }
  for (;;)
    {
      const auto line = read_line (socket);
      if (line.isEmpty ())
        {
          return false;
        }
      if (line == "\r\n" || line == "\n")
        {
          return true;
        }
      response.raw_headers += line;
      const auto colon = line.indexOf (':');
      if (colon > 0)
        {
          // Header names compare without case, so they are kept in lower case.
          response.headers.insert (
            line.left (colon).trimmed ().toLower (),
            line.mid (colon + 1).trimmed ());
        }
    }
}

inline void
McpHttpClient::read_body (QTcpSocket &socket, Response &response)
{
  const auto length = response.header ("Content-Length").toLongLong ();
  if (response.header ("Transfer-Encoding").toLower ().contains ("chunked"))
    {
      for (;;)
        {
          const auto size_line = read_line (socket);
          const auto digits = size_line.left (
            size_line.indexOf (';') < 0
              ? size_line.size ()
              : size_line.indexOf (';'));
          if (
            size_line.isEmpty () || digits.trimmed ().toLong (nullptr, 16) == 0)
            {
              return;
            }
          const auto size = digits.trimmed ().toLong (nullptr, 16);
          const auto complete = wait_until (socket, [&socket, size] {
            return socket.bytesAvailable () >= size;
          });
          if (!complete)
            {
              ADD_FAILURE () << "response body incomplete";
              return;
            }
          response.body += socket.read (size);
          read_line (socket);
        }
    }
  if (length > 0)
    {
      const auto complete = wait_until (socket, [&socket, length] {
        return socket.bytesAvailable () >= length;
      });
      if (!complete)
        {
          ADD_FAILURE () << "response body incomplete";
          return;
        }
      response.body += socket.read (length);
      return;
    }
  // A response without a length ends when the server closes the connection.
  wait_until (socket, [&socket] {
    return socket.state () == QAbstractSocket::UnconnectedState;
  });
  response.body += socket.readAll ();
}

} // namespace zrythm::mcp::test
