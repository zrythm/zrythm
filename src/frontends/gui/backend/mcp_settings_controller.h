// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "frontends/mcp/mcp_service.h"
#include "utils/app_settings.h"

#include <QObject>
#include <QPointer>
#include <QString>
#include <QVariantList>
#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::frontends::gui
{

/**
 * @brief The MCP server settings as the preferences page shows and
 * changes them.
 *
 * Reads mcpEnabled, mcpPort and mcpTokens from the settings and follows
 * their changes. Writes go through the invokables, which refuse a
 * change the endpoint cannot run with.
 *
 * tokens() holds one map per token with a "name" and a "grant" (the
 * grant identity). The stored hashes stay in the settings.
 *
 * settingsOverridden() reports whether the process environment takes
 * the place of the settings in this process (see
 * mcp::mcp_environment_override()).
 */
class McpSettingsController : public QObject
{
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY (
    zrythm::utils::AppSettings * appSettings READ appSettings WRITE
      setAppSettings NOTIFY appSettingsChanged)
  Q_PROPERTY (QVariantList tokens READ tokens NOTIFY tokensChanged)
  Q_PROPERTY (bool canEnable READ canEnable NOTIFY canEnableChanged)
  Q_PROPERTY (bool enabled READ enabled NOTIFY enabledChanged)
  Q_PROPERTY (int port READ port NOTIFY portChanged)
  Q_PROPERTY (QString endpointUrl READ endpointUrl NOTIFY portChanged)
  Q_PROPERTY (bool settingsOverridden READ settingsOverridden CONSTANT)
  Q_PROPERTY (bool overrideMalformed READ overrideMalformed CONSTANT)

public:
  explicit McpSettingsController (QObject * parent = nullptr);

  [[nodiscard]] utils::AppSettings * appSettings () const
  {
    return app_settings_;
  }
  void setAppSettings (utils::AppSettings * settings);

  /** The tokens as maps with a "name" and a "grant". */
  [[nodiscard]] QVariantList tokens () const { return tokens_; }

  /** Whether a token exists, which lets the server be enabled. */
  [[nodiscard]] bool canEnable () const { return !tokens_.isEmpty (); }

  /** Whether the server is set to run. */
  [[nodiscard]] bool enabled () const;

  /** The TCP port the server listens on. */
  [[nodiscard]] int port () const;

  /** The address MCP clients connect to. */
  [[nodiscard]] QString endpointUrl () const;

  /** Whether the process environment overrides the settings. */
  [[nodiscard]] bool settingsOverridden () const
  {
    return environment_override_ != mcp::McpEnvironmentOverride::None;
  }

  /** Whether the override is not valid, which keeps the server off. */
  [[nodiscard]] bool overrideMalformed () const
  {
    return environment_override_ == mcp::McpEnvironmentOverride::Malformed;
  }

  /**
   * Adds a token named @p name with the grant @p grantIdentity ("query",
   * "control" or "full") and returns its new secret.
   *
   * Returns an empty string when the name is refused (the reason is
   * nameError()) or the grant identity is unknown.
   */
  Q_INVOKABLE QString
  addToken (const QString &name, const QString &grantIdentity);

  /**
   * Removes the token at @p index of tokens().
   *
   * Removing the last token also turns the server off. Returns false
   * when the index is out of range or no settings are set.
   */
  Q_INVOKABLE bool removeToken (int index);

  /**
   * Turns the server on or off.
   *
   * Returns false when turning it on without a token, or when no
   * settings are set.
   */
  Q_INVOKABLE bool setEnabled (bool enabled);

  /**
   * Sets the port the server listens on.
   *
   * Returns false when @p port is outside 1 to 65535, or when no
   * settings are set.
   */
  Q_INVOKABLE bool setPort (int port);

  /** The reason @p name cannot name a new token, or an empty string. */
  Q_INVOKABLE QString nameError (const QString &name) const;

Q_SIGNALS:
  void appSettingsChanged ();
  void tokensChanged ();
  void canEnableChanged ();
  void enabledChanged ();
  void portChanged ();

private:
  /** The stored tokens, with the entries the parser drops left out. */
  [[nodiscard]] QList<mcp::McpToken> stored_tokens () const;

  /** Reads the stored tokens into tokens() and emits tokensChanged(). */
  void refresh_tokens ();

  QPointer<utils::AppSettings>      app_settings_;
  QVariantList                      tokens_;
  const mcp::McpEnvironmentOverride environment_override_;
};

} // namespace zrythm::frontends::gui
