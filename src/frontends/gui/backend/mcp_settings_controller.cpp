// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <utility>

#include "frontends/gui/backend/mcp_settings_controller.h"
#include "frontends/mcp/mcp_grant.h"
#include "utils/logger.h"
#include "utils/utf8_string.h"

#include <QVariantMap>

namespace zrythm::frontends::gui
{

McpSettingsController::McpSettingsController (QObject * parent)
    : QObject (parent), environment_override_ (mcp::mcp_environment_override ())
{
}

void
McpSettingsController::setAppSettings (utils::AppSettings * settings)
{
  if (app_settings_ == settings)
    {
      return;
    }
  if (app_settings_ != nullptr)
    {
      app_settings_->disconnect (this);
    }
  app_settings_ = settings;
  if (app_settings_ != nullptr)
    {
      connect (
        app_settings_, &utils::AppSettings::mcpTokensChanged, this,
        [this] (const QStringList &) { refresh_tokens (); });
      connect (
        app_settings_, &utils::AppSettings::mcpEnabledChanged, this,
        &McpSettingsController::enabledChanged);
      connect (
        app_settings_, &utils::AppSettings::mcpPortChanged, this,
        &McpSettingsController::portChanged);
    }
  refresh_tokens ();
  Q_EMIT appSettingsChanged ();
  Q_EMIT enabledChanged ();
  Q_EMIT portChanged ();
}

bool
McpSettingsController::enabled () const
{
  return app_settings_ != nullptr && app_settings_->mcpEnabled ();
}

int
McpSettingsController::port () const
{
  return app_settings_ != nullptr
           ? app_settings_->mcpPort ()
           : mcp::McpServiceConfig{}.port;
}

QString
McpSettingsController::endpointUrl () const
{
  return QStringLiteral ("http://127.0.0.1:%1/mcp").arg (port ());
}

QString
McpSettingsController::addToken (
  const QString &name,
  const QString &grantIdentity)
{
  const auto grant = mcp::grant_from_identity (grantIdentity);
  if (!grant.has_value ())
    {
      z_warning (
        "unknown MCP grant '{}'",
        utils::Utf8String::from_qstring (grantIdentity).view ());
      return {};
    }
  if (app_settings_ == nullptr)
    {
      return {};
    }
  auto tokens = stored_tokens ();
  if (const auto secret = mcp::add_token (tokens, name, *grant); secret)
    {
      app_settings_->set_mcpTokens (mcp::write_token_entries (tokens));
      return *secret;
    }
  return {};
}

bool
McpSettingsController::removeToken (int index)
{
  if (app_settings_ == nullptr)
    {
      return false;
    }
  auto tokens = stored_tokens ();
  if (index < 0 || static_cast<qsizetype> (index) >= tokens.size ())
    {
      return false;
    }
  tokens.erase (tokens.cbegin () + static_cast<qsizetype> (index));

  // The endpoint stays off without a token. Turning it off first keeps
  // the session from seeing an enabled setting with no tokens.
  if (tokens.isEmpty () && app_settings_->mcpEnabled ())
    {
      app_settings_->set_mcpEnabled (false);
    }
  app_settings_->set_mcpTokens (mcp::write_token_entries (tokens));
  return true;
}

bool
McpSettingsController::setEnabled (bool enabled)
{
  if (app_settings_ == nullptr || (enabled && stored_tokens ().isEmpty ()))
    {
      return false;
    }
  app_settings_->set_mcpEnabled (enabled);
  return true;
}

bool
McpSettingsController::setPort (int port)
{
  if (app_settings_ == nullptr || !mcp::is_valid_mcp_port (port))
    {
      return false;
    }
  app_settings_->set_mcpPort (port);
  return true;
}

QString
McpSettingsController::nameError (const QString &name) const
{
  const auto valid = mcp::validate_new_token_name (stored_tokens (), name);
  if (valid)
    {
      return {};
    }
  switch (valid.error ())
    {
    case mcp::TokenRejection::EmptyName:
      return tr ("Enter a name.");
    case mcp::TokenRejection::ForbiddenCharacter:
      return tr ("The name must not contain '|' or ':'.");
    case mcp::TokenRejection::SurroundingWhitespace:
      return tr ("The name must not start or end with a space.");
    case mcp::TokenRejection::DuplicateName:
      return tr ("A token with this name already exists.");
    }
  return {};
}

QList<mcp::McpToken>
McpSettingsController::stored_tokens () const
{
  if (app_settings_ == nullptr)
    {
      return {};
    }
  return mcp::parse_token_entries (app_settings_->mcpTokens ());
}

void
McpSettingsController::refresh_tokens ()
{
  const bool   was_enableable = canEnable ();
  QVariantList rows;
  for (const auto &token : stored_tokens ())
    {
      rows.append (
        QVariantMap{
          { "name",  token.name                        },
          { "grant", mcp::grant_identity (token.grant) }
      });
    }
  if (rows == tokens_)
    {
      return;
    }
  tokens_ = std::move (rows);
  Q_EMIT tokensChanged ();
  if (canEnable () != was_enableable)
    {
      Q_EMIT canEnableChanged ();
    }
}

} // namespace zrythm::frontends::gui
