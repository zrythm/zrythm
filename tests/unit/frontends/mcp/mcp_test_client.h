// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <chrono>
#include <cstring>

#include "utils/utf8_string.h"

#include <QByteArray>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QUrlQuery>

#include <gtest/gtest.h>

namespace zrythm::frontends::mcp::test
{

/**
 * A minimal MCP-over-HTTP client speaking raw TCP, for driving the
 * endpoint from tests.
 *
 * Each request opens its own connection; SSE streams get a dedicated
 * persistent socket. Waits run the event loop, so code queued on the
 * thread under test executes while a response is pending.
 */
class McpHttpClient
{
public:
  struct Response
  {
    int                          status = 0;
    QMap<QByteArray, QByteArray> headers;
    QByteArray                   body;

    [[nodiscard]] QJsonObject json () const
    {
      const auto document = QJsonDocument::fromJson (body);
      EXPECT_TRUE (document.isObject ());
      return document.object ();
    }
  };

  McpHttpClient (quint16 port, const QString &token)
      : port_ (port), token_ (token)
  {
  }

  ~McpHttpClient () { close_stream (); }

  /** Issues a POST /mcp with the given JSON-RPC message. */
  Response
  post (const QJsonObject &message, const QByteArray &method_header = {})
  {
    const auto body = QJsonDocument (message).toJson (QJsonDocument::Compact);
    QByteArray request;
    request += "POST /mcp HTTP/1.1\r\n";
    request += "Host: 127.0.0.1\r\n";
    request += "Content-Type: application/json\r\n";
    request += "Accept: application/json, text/event-stream\r\n";
    if (!method_header.isEmpty ())
      {
        request += "Mcp-Method: " + method_header + "\r\n";
      }
    if (!name_header_.isEmpty ())
      {
        request += "Mcp-Name: " + name_header_ + "\r\n";
      }
    if (!protocol_version_.isEmpty ())
      {
        request += "MCP-Protocol-Version: " + protocol_version_ + "\r\n";
      }
    if (!session_id_.isEmpty ())
      {
        request += "Mcp-Session-Id: " + session_id_ + "\r\n";
      }
    if (!token_.isEmpty ())
      {
        request += "Authorization: Bearer " + token_.toUtf8 () + "\r\n";
      }
    request += "Connection: close\r\n";
    request += "Content-Length: " + QByteArray::number (body.size ()) + "\r\n";
    request += "\r\n";
    request += body;

    return exchange (request, false);
  }

  /** Opens the legacy session stream: GET /mcp as SSE. */
  void open_stream ()
  {
    QByteArray request;
    request += "GET /mcp HTTP/1.1\r\n";
    request += "Host: 127.0.0.1\r\n";
    request += "Accept: text/event-stream\r\n";
    if (!session_id_.isEmpty ())
      {
        request += "Mcp-Session-Id: " + session_id_ + "\r\n";
      }
    request += "Authorization: Bearer " + token_.toUtf8 () + "\r\n";
    request += "\r\n";
    stream_ = exchange_socket (request, true);
  }

  void close_stream ()
  {
    if (stream_ != nullptr)
      {
        stream_->disconnectFromHost ();
        if (stream_->state () == QAbstractSocket::ConnectedState)
          {
            stream_->waitForDisconnected (1000);
          }
        delete stream_;
        stream_ = nullptr;
      }
  }

  /**
   * Waits for one SSE event on the open stream whose data parses as JSON
   * and satisfies the predicate.
   */
  template <typename Predicate>
  QJsonObject wait_for_event (
    Predicate                 predicate,
    std::chrono::milliseconds timeout = std::chrono::seconds (10))
  {
    Q_ASSERT (stream_ != nullptr);
    QTimer deadline;
    deadline.setSingleShot (true);
    deadline.start (timeout);
    while (stream_->canReadLine () || deadline.isActive ())
      {
        while (stream_->canReadLine ())
          {
            const auto line = stream_->readLine ();
            if (!line.startsWith ("data: "))
              {
                continue;
              }
            const auto payload = line.mid (strlen ("data: ")).trimmed ();
            const auto document = QJsonDocument::fromJson (payload);
            if (document.isObject () && predicate (document.object ()))
              {
                return document.object ();
              }
          }
        QEventLoop loop;
        QObject::connect (
          stream_, &QTcpSocket::readyRead, &loop, &QEventLoop::quit);
        QObject::connect (&deadline, &QTimer::timeout, &loop, &QEventLoop::quit);
        loop.exec ();
      }
    return {};
  }

  void set_session_id (const QByteArray &id) { session_id_ = id; }
  void set_protocol_version (const QByteArray &version)
  {
    protocol_version_ = version;
  }
  void set_name_header (const QByteArray &name) { name_header_ = name; }

  static quint16 pick_free_port ()
  {
    QTcpServer probe;
    probe.listen (QHostAddress::LocalHost, 0);
    const auto port = probe.serverPort ();
    probe.close ();
    return port;
  }

private:
  /** Sends @p request and reads one full HTTP response. */
  Response exchange (const QByteArray &request, bool keep_open)
  {
    auto *   socket = exchange_socket (request, keep_open);
    Response response;
    if (socket == nullptr)
      {
        return response;
      }
    if (!keep_open)
      {
        delete socket;
      }
    // headers and body were parsed by exchange_socket into pending_
    response = std::move (pending_);
    return response;
  }

  QTcpSocket * exchange_socket (const QByteArray &request, bool stream)
  {
    pending_ = Response{};
    auto * socket = new QTcpSocket ();
    socket->connectToHost (QHostAddress::LocalHost, port_);
    if (!socket->waitForConnected (5000))
      {
        ADD_FAILURE ()
          << "connect failed:"
          << utils::Utf8String::from_qstring (socket->errorString ());
        delete socket;
        return nullptr;
      }
    socket->write (request);
    socket->waitForBytesWritten (5000);

    // Read status line and headers.
    if (!wait_for (socket, [&] { return headers_end (socket) >= 0; }))
      {
        ADD_FAILURE () << "no HTTP response arrived";
        if (!stream)
          {
            delete socket;
            return nullptr;
          }
      }
    const auto header_bytes =
      socket->read (headers_end (socket) + strlen ("\r\n\r\n"));
    parse_headers (header_bytes);

    if (stream)
      {
        // The stream stays open; SSE events are read by wait_for_event().
        return socket;
      }

    // Read the body of a closed request/response exchange.
    const auto content_length =
      pending_.headers.value ("Content-Length").toInt ();
    if (!wait_for (socket, [&] {
          return socket->bytesAvailable () >= content_length;
        }))
      {
        ADD_FAILURE () << "response body incomplete";
      }
    pending_.body = socket->read (content_length);
    return socket;
  }

  template <typename Predicate>
  static bool wait_for (QTcpSocket * socket, Predicate predicate)
  {
    QTimer deadline;
    deadline.setSingleShot (true);
    deadline.start (10000);
    while (!predicate ())
      {
        if (!deadline.isActive ())
          {
            return false;
          }
        QEventLoop loop;
        QObject::connect (
          socket, &QTcpSocket::readyRead, &loop, &QEventLoop::quit);
        QObject::connect (&deadline, &QTimer::timeout, &loop, &QEventLoop::quit);
        loop.exec ();
      }
    return true;
  }

  static qint64 headers_end (QTcpSocket * socket)
  {
    // Peek for the blank line that terminates the header block.
    char       peek[8192];
    const auto available = socket->bytesAvailable ();
    const auto n = socket->peek (peek, std::min<qint64> (available, 8192));
    const QByteArray bytes (peek, int (n));
    return bytes.indexOf ("\r\n\r\n");
  }

  void parse_headers (const QByteArray &header_bytes)
  {
    const auto lines = header_bytes.split ('\n');
    bool       first = true;
    for (const auto &raw_line : lines)
      {
        auto line = raw_line;
        line.replace ("\r", "");
        if (first)
          {
            first = false;
            const auto parts = line.split (' ');
            if (parts.size () >= 2)
              {
                pending_.status = parts.at (1).toInt ();
              }
            continue;
          }
        const auto colon = line.indexOf (':');
        if (colon > 0)
          {
            pending_.headers.insert (
              line.left (colon).trimmed (), line.mid (colon + 1).trimmed ());
          }
      }
  }

  quint16      port_ = 0;
  QString      token_;
  QByteArray   session_id_;
  QByteArray   protocol_version_;
  QByteArray   name_header_;
  QTcpSocket * stream_ = nullptr;
  Response     pending_;
};

} // namespace zrythm::frontends::mcp::test
