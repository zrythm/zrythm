// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/app_settings.h"

#include <QSignalSpy>
#include <QStringList>
#include <QVariant>

#include "helpers/mock_settings_backend.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace ::testing;

namespace zrythm::utils
{

class AppSettingsTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    auto mock_backend = std::make_unique<test_helpers::MockSettingsBackend> ();
    mock_backend_ = mock_backend.get ();
    app_settings_ = std::make_unique<AppSettings> (std::move (mock_backend));
  }

  void TearDown () override { app_settings_.reset (); }

  test_helpers::MockSettingsBackend * mock_backend_{};
  std::unique_ptr<AppSettings>        app_settings_;
};

TEST_F (AppSettingsTest, Constructor)
{
  // Test that AppSettings can be constructed with a mock backend
  EXPECT_NE (app_settings_, nullptr);
}

TEST_F (AppSettingsTest, FirstRunProperty)
{
  // Test default value
  EXPECT_EQ (app_settings_->firstRunDefaultValue (), true);

  // Test getter returns default when backend has no value
  EXPECT_CALL (
    *mock_backend_, value (QAnyStringView ("firstRun"), QVariant (true)))
    .WillOnce (Return (QVariant (true)));
  EXPECT_EQ (app_settings_->firstRun (), true);

  // Test setter
  EXPECT_CALL (
    *mock_backend_, value (QAnyStringView ("firstRun"), QVariant (true)))
    .WillOnce (Return (QVariant (true)));
  EXPECT_CALL (
    *mock_backend_, setValue (QAnyStringView ("firstRun"), QVariant (false)))
    .Times (1);

  app_settings_->set_firstRun (false);
}

TEST_F (AppSettingsTest, FirstRunSignal)
{
  // Set up mock to return current value
  EXPECT_CALL (
    *mock_backend_, value (QAnyStringView ("firstRun"), QVariant (true)))
    .WillOnce (Return (QVariant (true)));

  // Set up mock for setValue
  EXPECT_CALL (
    *mock_backend_, setValue (QAnyStringView ("firstRun"), QVariant (false)))
    .Times (1);

  // Connect signal spy
  QSignalSpy spy (app_settings_.get (), &AppSettings::firstRunChanged);

  // Change value
  app_settings_->set_firstRun (false);

  // Verify signal was emitted
  EXPECT_EQ (spy.count (), 1);
  EXPECT_EQ (spy.at (0).at (0).toBool (), false);
}

TEST_F (AppSettingsTest, FirstRunNoSignalOnSameValue)
{
  // Set up mock to return same value we're setting
  EXPECT_CALL (
    *mock_backend_, value (QAnyStringView ("firstRun"), QVariant (true)))
    .WillOnce (Return (QVariant (true)));

  // setValue should not be called since values are equal
  EXPECT_CALL (*mock_backend_, setValue (QAnyStringView ("firstRun"), _))
    .Times (0);

  // Connect signal spy
  QSignalSpy spy (app_settings_.get (), &AppSettings::firstRunChanged);

  // Set same value
  app_settings_->set_firstRun (true);

  // Verify no signal was emitted
  EXPECT_EQ (spy.count (), 0);
}

TEST_F (AppSettingsTest, StringProperty)
{
  // Test iconTheme property
  QString expected_default = u"zrythm-dark"_s;
  EXPECT_EQ (app_settings_->iconThemeDefaultValue (), expected_default);

  // Test getter
  EXPECT_CALL (
    *mock_backend_,
    value (QAnyStringView ("iconTheme"), QVariant (expected_default)))
    .WillOnce (Return (QVariant (expected_default)));
  EXPECT_EQ (app_settings_->iconTheme (), expected_default);

  // Test setter
  EXPECT_CALL (
    *mock_backend_,
    value (QAnyStringView ("iconTheme"), QVariant (expected_default)))
    .WillOnce (Return (QVariant (expected_default)));
  EXPECT_CALL (
    *mock_backend_,
    setValue (QAnyStringView ("iconTheme"), QVariant (u"light-theme"_s)))
    .Times (1);

  app_settings_->set_iconTheme (u"light-theme"_s);
}

TEST_F (AppSettingsTest, ResetRestoresFactoryDefault)
{
  // The property declares RESET
  const QMetaObject * meta = app_settings_->metaObject ();
  const int           property_index = meta->indexOfProperty ("iconTheme");
  ASSERT_GE (property_index, 0);
  EXPECT_TRUE (meta->property (property_index).isResettable ());

  // Current value is a non-default theme
  EXPECT_CALL (
    *mock_backend_,
    value (QAnyStringView ("iconTheme"), QVariant (u"zrythm-dark"_s)))
    .WillOnce (Return (QVariant (u"light-theme"_s)));
  EXPECT_CALL (
    *mock_backend_,
    setValue (QAnyStringView ("iconTheme"), QVariant (u"zrythm-dark"_s)))
    .Times (1);

  QSignalSpy spy (app_settings_.get (), &AppSettings::iconThemeChanged);
  app_settings_->reset_iconTheme ();
  EXPECT_EQ (spy.count (), 1);
  EXPECT_EQ (spy.at (0).at (0).toString (), u"zrythm-dark"_s);
}

TEST_F (AppSettingsTest, ResetNoSignalWhenAlreadyDefault)
{
  EXPECT_CALL (
    *mock_backend_,
    value (QAnyStringView ("iconTheme"), QVariant (u"zrythm-dark"_s)))
    .WillOnce (Return (QVariant (u"zrythm-dark"_s)));
  EXPECT_CALL (*mock_backend_, setValue (QAnyStringView ("iconTheme"), _))
    .Times (0);

  QSignalSpy spy (app_settings_.get (), &AppSettings::iconThemeChanged);
  app_settings_->reset_iconTheme ();
  EXPECT_EQ (spy.count (), 0);
}

TEST_F (AppSettingsTest, StringListProperty)
{
  // Test recentProjects property
  QStringList expected_default;
  EXPECT_EQ (app_settings_->recentProjectsDefaultValue (), expected_default);

  QStringList test_list = { "project1", "project2", "project3" };

  // Test getter
  EXPECT_CALL (
    *mock_backend_,
    value (QAnyStringView ("recentProjects"), QVariant (expected_default)))
    .WillOnce (Return (QVariant (test_list)));
  EXPECT_EQ (app_settings_->recentProjects (), test_list);

  // Test setter
  EXPECT_CALL (
    *mock_backend_,
    value (QAnyStringView ("recentProjects"), QVariant (expected_default)))
    .WillOnce (Return (QVariant (expected_default)));
  EXPECT_CALL (
    *mock_backend_,
    setValue (QAnyStringView ("recentProjects"), QVariant (test_list)))
    .Times (1);

  app_settings_->set_recentProjects (test_list);
}

TEST_F (AppSettingsTest, NumericProperty)
{
  // Test undoStackLength property
  int expected_default = 128;
  EXPECT_EQ (app_settings_->undoStackLengthDefaultValue (), expected_default);

  // Test getter
  EXPECT_CALL (
    *mock_backend_,
    value (QAnyStringView ("undoStackLength"), QVariant (expected_default)))
    .WillOnce (Return (QVariant (256)));
  EXPECT_EQ (app_settings_->undoStackLength (), 256);

  // Test setter
  EXPECT_CALL (
    *mock_backend_,
    value (QAnyStringView ("undoStackLength"), QVariant (expected_default)))
    .WillOnce (Return (QVariant (expected_default)));
  EXPECT_CALL (
    *mock_backend_,
    setValue (QAnyStringView ("undoStackLength"), QVariant (512)))
    .Times (1);

  app_settings_->set_undoStackLength (512);
}

TEST_F (AppSettingsTest, DoubleProperty)
{
  // Test metronomeVolume property
  double expected_default = 1.0;
  EXPECT_EQ (app_settings_->metronomeVolumeDefaultValue (), expected_default);

  // Test getter
  EXPECT_CALL (
    *mock_backend_,
    value (QAnyStringView ("metronomeVolume"), QVariant (expected_default)))
    .WillOnce (Return (QVariant (0.5)));
  EXPECT_DOUBLE_EQ (app_settings_->metronomeVolume (), 0.5);

  // Test setter
  EXPECT_CALL (
    *mock_backend_,
    value (QAnyStringView ("metronomeVolume"), QVariant (expected_default)))
    .WillOnce (Return (QVariant (expected_default)));
  EXPECT_CALL (
    *mock_backend_,
    setValue (QAnyStringView ("metronomeVolume"), QVariant (1.5)))
    .Times (1);

  app_settings_->set_metronomeVolume (1.5);
}

TEST_F (AppSettingsTest, FloatProperty)
{
  // Test monitorVolume property
  float expected_default = 1.0f;
  EXPECT_FLOAT_EQ (
    app_settings_->monitorVolumeDefaultValue (), expected_default);

  // Test getter
  EXPECT_CALL (
    *mock_backend_,
    value (QAnyStringView ("monitorVolume"), QVariant (expected_default)))
    .WillOnce (Return (QVariant (0.75f)));
  EXPECT_FLOAT_EQ (app_settings_->monitorVolume (), 0.75f);

  // Test setter
  EXPECT_CALL (
    *mock_backend_,
    value (QAnyStringView ("monitorVolume"), QVariant (expected_default)))
    .WillOnce (Return (QVariant (expected_default)));
  EXPECT_CALL (
    *mock_backend_, setValue (QAnyStringView ("monitorVolume"), QVariant (0.5f)))
    .Times (1);

  app_settings_->set_monitorVolume (0.5f);
}

} // namespace zrythm::utils
