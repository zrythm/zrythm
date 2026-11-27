// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <concepts>
#include <expected>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <type_traits>
#include <utility>

#include "utils/logger.h"

#include <QtCore/QFuture>
#include <QtNetwork/QHostAddress>

#include <mcpserver/http_request.h>
#include <mcpserver/protocol_engine.h>

namespace zrythm::mcp
{

/** A type that a server can hold as the identity of a client. */
template <typename T>
concept ClientIdentity =
  std::is_object_v<T> && !std::is_array_v<T> && std::is_copy_constructible_v<T>;

/**
 * @brief An MCP server over Streamable HTTP that identifies its clients.
 *
 * The identity of a client is the value that the authenticator returns for a
 * request. The tool handler receives it with every tool call. All functions
 * must be called on the thread that owns this object.
 */
template <ClientIdentity Identity> class McpServer
{
public:
  /**
   * Identifies the client of a request. The server logs the error message
   * when it refuses the client.
   */
  using Authenticator =
    std::function<std::expected<Identity, std::string> (const HttpRequest &)>;

  /** What the tool handler knows about the request that it answers. */
  struct RequestContext
  {
    /** The identity of the client that sent the request. */
    const Identity  &identity;
    ProtocolRevision revision;
  };

  using CallToolHandler = std::function<QFuture<
    CallToolResult> (const ToolCall &call, const RequestContext &context)>;

  explicit McpServer (ServerOptions options)
      : engine_ (std::make_unique<ProtocolEngine> (std::move (options)))
  {
  }

  [[nodiscard]] auto options () const -> const ServerOptions &
  {
    return engine_->options ();
  }

  /** Sets the function that identifies the client of each request. */
  auto set_authenticator (Authenticator authenticator) -> void
  {
    z_return_if_fail (authenticator != nullptr);
    engine_->set_authenticator (
      [authenticator = std::move (authenticator)] (
        const HttpRequest &request) -> ProtocolEngine::Identity {
        auto identity = authenticator (request);
        if (!identity.has_value ())
          {
            z_warning ("MCP request refused: {}", identity.error ());
            return ProtocolEngine::Identity{};
          }
        return ProtocolEngine::Identity (std::move (*identity));
      });
  }

  /** The tools of the server, in the order in which they were added. */
  [[nodiscard]] auto tools () const -> std::span<const Tool>
  {
    return engine_->tools ();
  }

  /** Adds a tool. Returns an error when a tool with the same name exists. */
  [[nodiscard]] auto add_tool (Tool tool) -> std::expected<void, std::string>
  {
    return engine_->add_tool (std::move (tool));
  }

  /** Sets the function that calls a tool. */
  auto set_call_tool_handler (CallToolHandler handler) -> void
  {
    z_return_if_fail (handler != nullptr);
    engine_->set_call_tool_handler (
      [handler = std::move (handler)] (
        const ToolCall &call, const ProtocolRevision revision,
        const ProtocolEngine::Identity &identity) -> QFuture<CallToolResult> {
        const auto client_identity = std::any_cast<Identity> (&identity);
        z_return_val_if_fail (
          client_identity != nullptr, QFuture<CallToolResult>{});
        const RequestContext context{ *client_identity, revision };
        return handler (call, context);
      });
  }

  /** Adds a provider whose resources the server serves. The caller keeps the
   * ownership. */
  auto add_resource_provider (ResourceProvider &provider) -> void
  {
    engine_->add_resource_provider (provider);
  }

  /** Starts to serve on @p address and @p port. The port may be 0 to choose a
   * free port. */
  [[nodiscard]] auto listen (const QHostAddress &address, quint16 port)
    -> std::expected<void, std::string>
  {
    return engine_->listen (address, port);
  }

  /** Ends every stream and session and stops serving. */
  auto stop () -> void { engine_->stop (); }

  [[nodiscard]] auto is_listening () const -> bool
  {
    return engine_->is_listening ();
  }

  /** The port that the server serves on. It is 0 while the server does not
   * serve. */
  [[nodiscard]] auto listening_port () const -> quint16
  {
    return engine_->listening_port ();
  }

private:
  std::unique_ptr<ProtocolEngine> engine_;
};

} // namespace zrythm::mcp
