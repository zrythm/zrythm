// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <any>
#include <chrono>
#include <cstdint>
#include <expected>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>

#include <QtCore/QFuture>
#include <QtCore/QObject>
#include <QtNetwork/QHostAddress>

#include <mcpserver/mcp_types.h>

namespace zrythm::mcp
{

class HttpRequest;
class ResourceProvider;

/** Limits that a server enforces over all of its connections together. */
struct ServerLimits
{
  /** Largest body of a request. A larger body gets the status 413. */
  std::size_t max_request_body_bytes{ 1024 * 1024 };
  /**
   * Largest number of connections that the server holds open. A connection
   * over the limit gets the status 429. Each stream holds a connection, so
   * the value must cover every stream that the limits on streams, sessions
   * and streams per session allow.
   */
  std::uint32_t max_connections{ 128 };
  /** Largest number of listen streams that the server holds open. */
  std::size_t max_listen_streams{ 16 };
  /** Largest number of sessions that the server holds open. */
  std::size_t max_legacy_sessions{ 16 };
  /** Largest number of streams that one session holds open. */
  std::size_t max_streams_per_legacy_session{ 4 };
  /** A session ends after it had no request and no open stream for this long. */
  std::chrono::milliseconds legacy_session_idle_timeout{ std::chrono::minutes{
    30 } };
  /** Time between two keep-alive comments on an open stream. */
  std::chrono::milliseconds stream_keep_alive_interval{ std::chrono::seconds{
    15 } };
  /** Time between two checks for clients that closed their connection. */
  std::chrono::milliseconds stream_liveness_interval{ std::chrono::seconds{
    1 } };
};

/** The settings of a server. */
struct ServerOptions
{
  /** The name and the version that the server reports. */
  Implementation server;
  /** Text that the server shows to the model. */
  std::optional<std::string> instructions;
  /** The path of the endpoint of the server. */
  std::string endpoint_path{ "/mcp" };
  /** The limits of the server. */
  ServerLimits limits;
};

/**
 * @brief The protocol core of an MCP server over Streamable HTTP.
 *
 * The engine handles no identity: it holds the identity of each request as
 * an erased value and passes it to the handlers. All functions must be
 * called on the thread that owns this object. Tool and resource handlers
 * return futures, so the thread never waits for a result.
 */
class ProtocolEngine final : public QObject
{
  Q_OBJECT

public:
  /**
   * @brief The identity of the client of a request, as the authenticator
   * returns it.
   *
   * The engine reads nothing of the value. An empty value means that the
   * client is refused.
   */
  using Identity = std::any;
  /** Returns an empty identity when the client of the request is refused. */
  using Authenticator = std::function<Identity (const HttpRequest &)>;
  using CallToolHandler = std::function<QFuture<CallToolResult> (
    const ToolCall  &call,
    ProtocolRevision revision,
    const Identity  &identity)>;

  explicit ProtocolEngine (ServerOptions options, QObject * parent = nullptr);
  ~ProtocolEngine () override;
  ProtocolEngine (const ProtocolEngine &) = delete;
  ProtocolEngine &operator= (const ProtocolEngine &) = delete;
  ProtocolEngine (ProtocolEngine &&) = delete;
  ProtocolEngine &operator= (ProtocolEngine &&) = delete;

  [[nodiscard]] auto options () const -> const ServerOptions &;

  /** Sets the function that identifies the client of each request. */
  auto set_authenticator (Authenticator authenticator) -> void;

  /** The tools of the server, in the order in which they were added. */
  [[nodiscard]] auto tools () const -> std::span<const Tool>;

  /** Adds a tool. Returns an error when a tool with the same name exists. */
  [[nodiscard]] auto add_tool (Tool tool) -> std::expected<void, std::string>;

  /** Sets the function that calls a tool. */
  auto set_call_tool_handler (CallToolHandler handler) -> void;

  /**
   * @brief Adds a provider whose resources the server serves.
   *
   * The caller keeps the ownership of @p provider. A provider that is
   * destroyed stops to be served.
   */
  auto add_resource_provider (ResourceProvider &provider) -> void;

  /**
   * @brief Starts to serve on @p address and @p port.
   *
   * The port may be 0 to choose a free port. Returns an error when the
   * server has no authenticator, when it has tools but no tool handler,
   * when two providers serve the same URI, or when max_connections covers
   * fewer connections than the limits on streams and sessions allow.
   */
  [[nodiscard]] auto listen (const QHostAddress &address, quint16 port)
    -> std::expected<void, std::string>;

  /** Ends every stream and session and stops serving. */
  auto stop () -> void;

  [[nodiscard]] auto is_listening () const -> bool;

  /** The port that the server serves on. It is 0 while the server does not
   * serve. */
  [[nodiscard]] auto listening_port () const -> quint16;

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace zrythm::mcp
