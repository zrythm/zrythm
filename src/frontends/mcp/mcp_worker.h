// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <functional>
#include <memory>
#include <optional>

#include "frontends/mcp/mcp_grant.h"
#include "frontends/mcp/mcp_tool_catalog.h"

#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QPromise>
#include <QString>
#include <QUrl>
#include <QUuid>
#include <QtMcpCommon/QMcpCallToolResult>

class QMcpServer;
class QMcpServerSession;
class QMcpServerStreamableHttp;

namespace zrythm::frontends::mcp
{

/** One call routed to the main thread, as the worker hands it over. */
struct McpDispatchRequest
{
  QString     tool;
  QJsonObject arguments;

  /** The expected-revision argument the client sent, if any. */
  std::optional<std::uint64_t> expected_revision = std::nullopt;

  /** Fulfilled on the worker thread when the operation completes. */
  std::shared_ptr<QPromise<QMcpCallToolResult>> completion;
};

using McpDispatchFn = std::function<void (const McpDispatchRequest &)>;

/**
 * Runs the MCP server and its streamable HTTP transport on the endpoint's
 * worker thread.
 *
 * The object is created on the main thread, moved to the worker thread,
 * and every slot runs there. Tool calls are validated against the
 * connection's grant and the catalog, then handed to the dispatch function,
 * which marshals them to the main thread; the result is marshaled back and
 * fulfills the call's future on this thread, keeping all qtmcp calls
 * single-threaded.
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
   * The listen address is always loopback; @p port is its TCP port.
   *
   * @c token_hashes maps the hex SHA-256 of each accepted bearer token to
   * the grant it carries. The server refuses to start without at least one
   * token.
   */
  struct Config
  {
    int                      port = 0;
    QHash<QString, McpGrant> token_hashes;
    McpToolCatalog           catalog;

    /** The journal entries the resource starts with, oldest first. */
    QJsonArray initial_journal_entries;
  };

  explicit McpWorkerController (McpDispatchFn dispatch_fn);
  ~McpWorkerController () override;

  /** Creates and starts the server; runs on the worker thread. */
  void start_server (Config config);

  /** Fulfills a call marshaled back from the main thread. */
  void fulfill_call (
    std::shared_ptr<QPromise<QMcpCallToolResult>> completion,
    QMcpCallToolResult                            result);

  /** Appends a journal entry and updates every session's journal resource. */
  void push_journal_entry (QJsonObject entry);

Q_SIGNALS:
  void server_started ();
  void server_error (const QString &message);

private:
  void                append_journal_resource (QMcpServerSession * session);
  void                update_journal_resources ();
  QMcpServerSession * find_session (const QUuid &session_id) const;

  McpDispatchFn          dispatch_fn_;
  Config                 config_;
  QMcpServer *           server_ = nullptr;
  QHash<QUuid, McpGrant> session_grants_;
  QJsonArray             journal_entries_;
};

} // namespace zrythm::frontends::mcp
