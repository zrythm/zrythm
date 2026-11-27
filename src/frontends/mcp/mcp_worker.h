// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>

#include "frontends/mcp/mcp_grant.h"
#include "frontends/mcp/mcp_tool_catalog.h"

#include <QHash>
#include <QObject>
#include <QPromise>
#include <QString>

#include <mcpserver/mcp_server.h>
#include <nlohmann/json.hpp>

namespace zrythm::frontends::mcp
{

/** The identity of a connection: the grant of the token it authenticated with. */
struct TokenIdentity
{
  McpGrant grant = McpGrant::Query;
};

/** One call routed to the main thread, as the worker hands it over. */
struct McpDispatchRequest
{
  std::string tool;

  /** The arguments of the call, without the expected-revision argument. */
  nlohmann::json arguments;

  /** The expected-revision argument the client sent, if any. */
  std::optional<std::uint64_t> expected_revision;

  /** Fulfilled on the worker thread when the operation completes. */
  std::shared_ptr<QPromise<zrythm::mcp::CallToolResult>> completion;
};

using McpDispatchFn = std::function<void (const McpDispatchRequest &)>;

/**
 * Runs the MCP server and its streamable HTTP transport on the endpoint's
 * worker thread.
 *
 * The object is created on the main thread, moved to the worker thread,
 * and every slot runs there. Each request is identified by the grant of
 * its bearer token. A tool call that the grant allows is handed to the
 * dispatch function, which marshals it to the main thread; the result is
 * marshaled back and fulfills the call's future on this thread.
 *
 * Calls still awaiting a result when the worker is destroyed are
 * abandoned: their futures never resolve, which is safe because the
 * transport that would carry the response is gone with them.
 */
class McpWorkerController : public QObject
{
  Q_OBJECT

public:
  /**
   * The listen address is always loopback; @c port is its TCP port.
   *
   * @c token_hashes maps the lowercase hex SHA-256 of each accepted
   * bearer token to the grant it carries. The server refuses to start
   * without at least one token.
   */
  struct Config
  {
    quint16                  port = 0;
    QHash<QString, McpGrant> token_hashes;
    McpToolCatalog           catalog;
  };

  explicit McpWorkerController (McpDispatchFn dispatch_fn);
  ~McpWorkerController () override;

  /** Creates and starts the server; runs on the worker thread. */
  void start_server (Config config);

  /** Fulfills a call marshaled back from the main thread. */
  void fulfill_call (
    std::shared_ptr<QPromise<zrythm::mcp::CallToolResult>> completion,
    zrythm::mcp::CallToolResult                            result);

  /**
   * @brief Appends @p entry to the journal resource.
   *
   * The subscribers of the resource are told that it changed. Entries
   * that arrive before the server exists are kept for when it starts.
   */
  void push_journal_entry (nlohmann::json entry);

Q_SIGNALS:
  void server_started ();
  void server_error (const QString &message);

private:
  using Server = zrythm::mcp::McpServer<TokenIdentity>;

  /** The undo history journal as the resource zrythm://journal. */
  class JournalResource;

  McpDispatchFn dispatch_fn_;
  Config        config_;

  /** Declared before the server, so that the server is destroyed first. */
  std::unique_ptr<JournalResource> journal_;
  std::unique_ptr<Server>          server_;
};

} // namespace zrythm::frontends::mcp
