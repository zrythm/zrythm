// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <optional>

#include "frontends/mcp/mcp_grant.h"
#include "frontends/mcp/mcp_worker.h"

#include <QJsonArray>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>

namespace zrythm::operations
{
class OperationDispatcher;
class OperationRegistry;
class OperationReport;
} // namespace zrythm::operations

namespace zrythm::undo
{
class UndoStack;
}

class QThread;

namespace zrythm::frontends::mcp
{

/** One named access token. The secret is stored only as its SHA-256. */
struct McpToken
{
  QString  name;
  McpGrant grant = McpGrant::Query;

  /** Lowercase hex SHA-256 of the secret. */
  QString sha256_hex;

  /** Hashes the secret into a token. */
  static McpToken
  from_secret (const QString &name, McpGrant grant, const QString &secret);

  bool operator== (const McpToken &) const = default;
};

/** The endpoint configuration; an empty token list keeps the endpoint off. */
struct McpServiceConfig
{
  bool enabled = false;

  /** TCP port on 127.0.0.1. */
  int port = 7777;

  QList<McpToken> tokens;
};

/**
 * The MCP frontend of one project session.
 *
 * Lives on the main thread. It owns the worker thread the server and its
 * HTTP transport run on, marshals every tool call to the main thread for
 * dispatch through the operation dispatcher, and forwards undo stack
 * journal entries to the connected clients.
 */
class McpService : public QObject
{
  Q_OBJECT

public:
  McpService (
    const operations::OperationRegistry &registry,
    operations::OperationDispatcher     &dispatcher,
    undo::UndoStack                     &undo_stack,
    QObject *                            parent = nullptr);
  ~McpService () override;

  /**
   * Applies the configuration, starting, restarting or stopping the
   * endpoint as needed.
   *
   * The endpoint starts only when enabled with at least one token; an
   * enabled configuration without tokens is refused with a warning,
   * because a tokenless endpoint would accept unauthenticated callers.
   */
  void apply_config (const McpServiceConfig &config);

  [[nodiscard]] bool is_running () const { return worker_ != nullptr; }

Q_SIGNALS:
  void started ();
  void error_occurred (const QString &message);
  void stopped ();

private:
  void start ();
  void stop ();
  void run_dispatch (const McpDispatchRequest &request);

  const operations::OperationRegistry &registry_;
  operations::OperationDispatcher     &dispatcher_;
  undo::UndoStack                     &undo_stack_;

  McpServiceConfig            config_;
  QThread *                   thread_ = nullptr;
  class McpWorkerController * worker_ = nullptr;
};

/**
 * Parses settings entries of the form "name|grant|sha256hex".
 *
 * Invalid entries are skipped with a warning.
 */
QList<McpToken>
parse_token_entries (const QStringList &entries);

/**
 * Builds the endpoint configuration from the process environment.
 *
 * A ZRYTHM_MCP_TOKEN of the form "name:grant:secret" enables the
 * endpoint with that token; ZRYTHM_MCP_PORT overrides the listen port.
 * Returns no value when no token is set. A malformed token or port is
 * authoritative: it returns a disabled configuration instead of
 * falling back to the settings, so the endpoint never runs with
 * credentials the environment did not name.
 */
std::optional<McpServiceConfig>
mcp_config_from_environment ();

} // namespace zrythm::frontends::mcp
