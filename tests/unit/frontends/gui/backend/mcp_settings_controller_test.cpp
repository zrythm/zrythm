// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "frontends/gui/backend/mcp_settings_controller.h"
#include "frontends/mcp/mcp_service.h"
#include "utils/app_settings.h"

#include <QSet>
#include <QSignalSpy>
#include <QStringList>
#include <QVariantMap>

#include "helpers/in_memory_settings_backend.h"
#include "helpers/scoped_env_var.h"
#include "helpers/scoped_qcoreapplication.h"

#include <gtest/gtest.h>

namespace zrythm::frontends::gui
{

class McpSettingsControllerTest
    : public ::testing::Test,
      public test_helpers::ScopedQCoreApplication
{
protected:
  void SetUp () override
  {
    auto backend = std::make_unique<test_helpers::InMemorySettingsBackend> ();
    backend_ = backend.get ();
    settings_ = std::make_unique<utils::AppSettings> (
      std::move (backend), utils::UserDirectories{});
    controller_ = std::make_unique<McpSettingsController> ();
    controller_->setAppSettings (settings_.get ());
  }

  /** The settings entry of a token with @p secret. */
  static QString
  token_entry (const QString &name, mcp::McpGrant grant, const QString &secret)
  {
    const QList<mcp::McpToken> tokens{
      mcp::McpToken::from_secret (name, grant, secret)
    };
    return mcp::write_token_entries (tokens).front ();
  }

  /** The row the controller shows for a token. */
  static QVariantMap token_row (const QString &name, const QString &grant)
  {
    return QVariantMap{
      { QStringLiteral ("name"),  QVariant (name)  },
      { QStringLiteral ("grant"), QVariant (grant) }
    };
  }

  /** The stored SHA-256 of @p secret. */
  static QString hash_of (const QString &secret)
  {
    return mcp::McpToken::from_secret (QString (), mcp::McpGrant::Query, secret)
      .sha256_hex;
  }

  /** Whether a value was written to the setting @p key. */
  [[nodiscard]] bool has_written (const char * key) const
  {
    return backend_->value (key, QVariant ()).isValid ();
  }

  test_helpers::InMemorySettingsBackend * backend_{};
  std::unique_ptr<utils::AppSettings>     settings_;
  std::unique_ptr<McpSettingsController>  controller_;
};

TEST_F (McpSettingsControllerTest, AddTokenStoresTheHashAndReturnsTheSecret)
{
  QSignalSpy tokens_changed (
    controller_.get (), &McpSettingsController::tokensChanged);
  QSignalSpy can_enable_changed (
    controller_.get (), &McpSettingsController::canEnableChanged);

  const auto secret =
    controller_->addToken (QStringLiteral ("studio"), QStringLiteral ("full"));
  ASSERT_EQ (secret.size (), 64);

  // Only the hash of the secret reaches the settings.
  const auto entries = settings_->mcpTokens ();
  ASSERT_EQ (entries.size (), 1);
  EXPECT_EQ (
    entries.front (), QStringLiteral ("studio|full|%1").arg (hash_of (secret)));

  const auto tokens = controller_->tokens ();
  ASSERT_EQ (tokens.size (), 1);
  EXPECT_EQ (
    tokens.front ().toMap (),
    token_row (QStringLiteral ("studio"), QStringLiteral ("full")));
  EXPECT_TRUE (controller_->canEnable ());
  EXPECT_EQ (tokens_changed.size (), 1);
  EXPECT_EQ (can_enable_changed.size (), 1);
}

TEST_F (McpSettingsControllerTest, AddTokenRefusesAnInvalidName)
{
  EXPECT_TRUE (
    controller_->addToken (QString (), QStringLiteral ("full")).isEmpty ());
  EXPECT_TRUE (
    controller_->addToken (QStringLiteral ("a|b"), QStringLiteral ("full"))
      .isEmpty ());
  EXPECT_FALSE (has_written ("mcpTokens"));
  EXPECT_TRUE (controller_->tokens ().isEmpty ());
}

TEST_F (McpSettingsControllerTest, RemoveLastTokenDisablesFirst)
{
  settings_->set_mcpTokens (
    { token_entry (QStringLiteral ("studio"), mcp::McpGrant::Full, "s") });
  ASSERT_TRUE (controller_->setEnabled (true));
  ASSERT_TRUE (controller_->enabled ());

  QStringList order;
  QObject::connect (
    settings_.get (), &utils::AppSettings::mcpEnabledChanged, controller_.get (),
    [&order] (bool) { order.append (QStringLiteral ("enabled")); });
  QObject::connect (
    settings_.get (), &utils::AppSettings::mcpTokensChanged, controller_.get (),
    [&order] (const QStringList &) {
      order.append (QStringLiteral ("tokens"));
    });

  EXPECT_TRUE (controller_->removeToken (0));
  EXPECT_EQ (
    order,
    QStringList ({ QStringLiteral ("enabled"), QStringLiteral ("tokens") }));
  EXPECT_FALSE (settings_->mcpEnabled ());
  EXPECT_TRUE (settings_->mcpTokens ().isEmpty ());
  EXPECT_FALSE (controller_->enabled ());
  EXPECT_TRUE (controller_->tokens ().isEmpty ());
  EXPECT_FALSE (controller_->canEnable ());
}

TEST_F (McpSettingsControllerTest, RemoveTokenKeepsTheOtherEntries)
{
  const auto kept =
    token_entry (QStringLiteral ("kept"), mcp::McpGrant::Query, "k");
  const auto gone =
    token_entry (QStringLiteral ("gone"), mcp::McpGrant::Full, "g");
  settings_->set_mcpTokens ({ kept, gone });

  // An index outside the list is refused and writes nothing.
  EXPECT_FALSE (controller_->removeToken (-1));
  EXPECT_FALSE (controller_->removeToken (2));
  EXPECT_EQ (settings_->mcpTokens (), QStringList ({ kept, gone }));

  EXPECT_TRUE (controller_->removeToken (1));
  EXPECT_EQ (settings_->mcpTokens (), QStringList ({ kept }));
  EXPECT_FALSE (has_written ("mcpEnabled"));
  ASSERT_EQ (controller_->tokens ().size (), 1);
  EXPECT_EQ (
    controller_->tokens ().front ().toMap (),
    token_row (QStringLiteral ("kept"), QStringLiteral ("query")));
}

TEST_F (McpSettingsControllerTest, SetEnabledWithoutTokenIsRefused)
{
  EXPECT_FALSE (controller_->setEnabled (true));
  EXPECT_FALSE (has_written ("mcpEnabled"));
  EXPECT_FALSE (controller_->enabled ());

  controller_->addToken (QStringLiteral ("studio"), QStringLiteral ("full"));
  EXPECT_TRUE (controller_->setEnabled (true));
  EXPECT_TRUE (settings_->mcpEnabled ());
  EXPECT_TRUE (controller_->enabled ());

  EXPECT_TRUE (controller_->setEnabled (false));
  EXPECT_FALSE (settings_->mcpEnabled ());
  EXPECT_FALSE (controller_->enabled ());
}

TEST_F (McpSettingsControllerTest, SetPortOutOfRangeIsRefused)
{
  for (const auto port : { -1, 0, 65536 })
    {
      EXPECT_FALSE (controller_->setPort (port));
    }
  EXPECT_FALSE (has_written ("mcpPort"));
  EXPECT_EQ (controller_->port (), settings_->mcpPortDefaultValue ());
  EXPECT_EQ (
    controller_->endpointUrl (),
    QStringLiteral ("http://127.0.0.1:%1/mcp")
      .arg (settings_->mcpPortDefaultValue ()));

  EXPECT_TRUE (controller_->setPort (8080));
  EXPECT_EQ (settings_->mcpPort (), 8080);
  EXPECT_EQ (controller_->port (), 8080);
  EXPECT_EQ (
    controller_->endpointUrl (), QStringLiteral ("http://127.0.0.1:8080/mcp"));
}

TEST_F (McpSettingsControllerTest, TokensFollowSettings)
{
  QSignalSpy tokens_changed (
    controller_.get (), &McpSettingsController::tokensChanged);

  settings_->set_mcpTokens (
    { token_entry (QStringLiteral ("reader"), mcp::McpGrant::Query, "a"),
      // Dropped by the parser, so it gets no row.
      QStringLiteral ("malformed"),
      token_entry (QStringLiteral ("editor"), mcp::McpGrant::Full, "b") });
  EXPECT_EQ (tokens_changed.size (), 1);
  const auto tokens = controller_->tokens ();
  ASSERT_EQ (tokens.size (), 2);
  EXPECT_EQ (
    tokens.at (0).toMap (),
    token_row (QStringLiteral ("reader"), QStringLiteral ("query")));
  EXPECT_EQ (
    tokens.at (1).toMap (),
    token_row (QStringLiteral ("editor"), QStringLiteral ("full")));

  settings_->set_mcpTokens ({});
  EXPECT_EQ (tokens_changed.size (), 2);
  EXPECT_TRUE (controller_->tokens ().isEmpty ());
}

TEST_F (
  McpSettingsControllerTest,
  SettingsSwapRereadsAndStopsFollowingTheOldSettings)
{
  settings_->set_mcpTokens (
    { token_entry (QStringLiteral ("old"), mcp::McpGrant::Query, "o") });
  auto other = std::make_unique<utils::AppSettings> (
    std::make_unique<test_helpers::InMemorySettingsBackend> (),
    utils::UserDirectories{});
  other->set_mcpTokens (
    { token_entry (QStringLiteral ("new"), mcp::McpGrant::Full, "n") });

  controller_->setAppSettings (other.get ());
  ASSERT_EQ (controller_->tokens ().size (), 1);
  EXPECT_EQ (
    controller_->tokens ().front ().toMap (),
    token_row (QStringLiteral ("new"), QStringLiteral ("full")));

  settings_->set_mcpTokens ({});
  ASSERT_EQ (controller_->tokens ().size (), 1);
  EXPECT_EQ (
    controller_->tokens ().front ().toMap (),
    token_row (QStringLiteral ("new"), QStringLiteral ("full")));
}

TEST_F (McpSettingsControllerTest, NameErrorIsEmptyForAUsableName)
{
  controller_->addToken (QStringLiteral ("studio"), QStringLiteral ("full"));

  QSet<QString> reasons;
  for (
    const auto &name :
    { QString (), QStringLiteral ("a|b"), QStringLiteral (" studio"),
      QStringLiteral ("studio") })
    {
      const auto reason = controller_->nameError (name);
      ASSERT_FALSE (reason.isEmpty ());
      reasons.insert (reason);
    }
  // Each refusal has its own message.
  EXPECT_EQ (reasons.size (), 4);
  EXPECT_TRUE (controller_->nameError (QStringLiteral ("desk")).isEmpty ());
}

TEST_F (McpSettingsControllerTest, WithoutSettingsReadsDefaultsAndRefusesWrites)
{
  McpSettingsController controller;

  EXPECT_TRUE (controller.tokens ().isEmpty ());
  EXPECT_FALSE (controller.canEnable ());
  EXPECT_FALSE (controller.enabled ());
  EXPECT_EQ (controller.port (), mcp::McpServiceConfig{}.port);
  EXPECT_EQ (
    controller.endpointUrl (), QStringLiteral ("http://127.0.0.1:7777/mcp"));

  EXPECT_TRUE (
    controller.addToken (QStringLiteral ("s"), QStringLiteral ("full"))
      .isEmpty ());
  EXPECT_FALSE (controller.removeToken (0));
  EXPECT_FALSE (controller.setEnabled (true));
  EXPECT_FALSE (controller.setEnabled (false));
  EXPECT_FALSE (controller.setPort (8080));
  EXPECT_FALSE (controller.nameError (QString ()).isEmpty ());
}

TEST_F (McpSettingsControllerTest, EnvironmentOverrideIsReadAtConstruction)
{
  // Restores the variables on any exit, including assertion failures.
  test_helpers::ScopedEnvVar token_guard ("ZRYTHM_MCP_TOKEN");
  test_helpers::ScopedEnvVar port_guard ("ZRYTHM_MCP_PORT");
  qunsetenv ("ZRYTHM_MCP_TOKEN");
  qunsetenv ("ZRYTHM_MCP_PORT");
  {
    const McpSettingsController controller;
    EXPECT_FALSE (controller.settingsOverridden ());
    EXPECT_FALSE (controller.overrideMalformed ());
  }

  qputenv ("ZRYTHM_MCP_TOKEN", QByteArray ("ci:full:hunter2"));
  {
    const McpSettingsController controller;
    EXPECT_TRUE (controller.settingsOverridden ());
    EXPECT_FALSE (controller.overrideMalformed ());
  }

  qputenv ("ZRYTHM_MCP_PORT", QByteArray ("70000"));
  {
    const McpSettingsController controller;
    EXPECT_TRUE (controller.settingsOverridden ());
    EXPECT_TRUE (controller.overrideMalformed ());
  }
}

} // namespace zrythm::frontends::gui
