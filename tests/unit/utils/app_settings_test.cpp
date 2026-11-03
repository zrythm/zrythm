// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/app_settings.h"

#include <QSignalSpy>
#include <QStringList>
#include <QVariant>

#include "helpers/in_memory_settings_backend.h"
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

TEST_F (AppSettingsTest, ResetRestoresFactoryDefault)
{
  // The property declares RESET
  const QMetaObject * meta = app_settings_->metaObject ();
  const int           property_index = meta->indexOfProperty ("accentToken");
  ASSERT_GE (property_index, 0);
  EXPECT_TRUE (meta->property (property_index).isResettable ());

  // Current value is a non-default accent
  EXPECT_CALL (
    *mock_backend_,
    value (QAnyStringView ("accentToken"), QVariant (u"zrythm-orange"_s)))
    .WillOnce (Return (QVariant (u"gunmetal"_s)));
  EXPECT_CALL (
    *mock_backend_,
    setValue (QAnyStringView ("accentToken"), QVariant (u"zrythm-orange"_s)))
    .Times (1);

  QSignalSpy spy (app_settings_.get (), &AppSettings::accentTokenChanged);
  app_settings_->reset_accentToken ();
  EXPECT_EQ (spy.count (), 1);
  EXPECT_EQ (spy.at (0).at (0).toString (), u"zrythm-orange"_s);
}

TEST_F (AppSettingsTest, ResetNoSignalWhenAlreadyDefault)
{
  EXPECT_CALL (
    *mock_backend_,
    value (QAnyStringView ("accentToken"), QVariant (u"zrythm-orange"_s)))
    .WillOnce (Return (QVariant (u"zrythm-orange"_s)));
  EXPECT_CALL (*mock_backend_, setValue (QAnyStringView ("accentToken"), _))
    .Times (0);

  QSignalSpy spy (app_settings_.get (), &AppSettings::accentTokenChanged);
  app_settings_->reset_accentToken ();
  EXPECT_EQ (spy.count (), 0);
}

TEST_F (AppSettingsTest, ThemeModeProperty)
{
  const auto default_value = AppSettings::ThemeMode::Dark;
  EXPECT_EQ (app_settings_->themeModeDefaultValue (), default_value);

  EXPECT_CALL (
    *mock_backend_,
    value (QAnyStringView ("themeMode"), QVariant::fromValue (default_value)))
    .WillOnce (Return (QVariant::fromValue (AppSettings::ThemeMode::Light)));
  EXPECT_EQ (app_settings_->themeMode (), AppSettings::ThemeMode::Light);

  EXPECT_CALL (
    *mock_backend_,
    value (QAnyStringView ("themeMode"), QVariant::fromValue (default_value)))
    .WillOnce (Return (QVariant::fromValue (default_value)));
  EXPECT_CALL (
    *mock_backend_,
    setValue (
      QAnyStringView ("themeMode"),
      QVariant::fromValue (AppSettings::ThemeMode::Light)))
    .Times (1);

  app_settings_->set_themeMode (AppSettings::ThemeMode::Light);
}

TEST_F (AppSettingsTest, AccentTokenProperty)
{
  const QString default_value = u"zrythm-orange"_s;
  EXPECT_EQ (app_settings_->accentTokenDefaultValue (), default_value);

  EXPECT_CALL (
    *mock_backend_,
    value (QAnyStringView ("accentToken"), QVariant (default_value)))
    .WillOnce (Return (QVariant (u"celestial-blue"_s)));
  EXPECT_EQ (app_settings_->accentToken (), u"celestial-blue"_s);

  EXPECT_CALL (
    *mock_backend_,
    value (QAnyStringView ("accentToken"), QVariant (default_value)))
    .WillOnce (Return (QVariant (default_value)));
  EXPECT_CALL (
    *mock_backend_,
    setValue (QAnyStringView ("accentToken"), QVariant (u"gunmetal"_s)))
    .Times (1);

  app_settings_->set_accentToken (u"gunmetal"_s);
}

TEST_F (AppSettingsTest, DockLayoutProperty)
{
  const QString default_value;
  const QString test_layout = u"{\"version\":1}"_s;
  EXPECT_EQ (app_settings_->dockLayoutDefaultValue (), default_value);

  EXPECT_CALL (
    *mock_backend_,
    value (QAnyStringView ("dockLayout"), QVariant (default_value)))
    .WillOnce (Return (QVariant (test_layout)));
  EXPECT_EQ (app_settings_->dockLayout (), test_layout);

  EXPECT_CALL (
    *mock_backend_,
    value (QAnyStringView ("dockLayout"), QVariant (default_value)))
    .WillOnce (Return (QVariant (default_value)));
  EXPECT_CALL (
    *mock_backend_,
    setValue (QAnyStringView ("dockLayout"), QVariant (test_layout)))
    .Times (1);

  app_settings_->set_dockLayout (test_layout);

  EXPECT_CALL (
    *mock_backend_,
    value (QAnyStringView ("dockLayout"), QVariant (default_value)))
    .WillOnce (Return (QVariant (test_layout)));
  EXPECT_CALL (
    *mock_backend_,
    setValue (QAnyStringView ("dockLayout"), QVariant (default_value)))
    .Times (1);

  app_settings_->reset_dockLayout ();
}

TEST_F (AppSettingsTest, ResetPreferenceSettingsRestoresOnlyPreferenceRows)
{
  auto   backend = std::make_unique<test_helpers::InMemorySettingsBackend> ();
  auto * backend_ptr = backend.get ();
  AppSettings settings (std::move (backend));

  settings.set_themeMode (AppSettings::ThemeMode::Light);
  settings.set_accentToken (u"gunmetal"_s);
  settings.set_uiLocale (u"fr"_s);
  settings.set_transportReturnToCue (false);
  settings.set_startPlaybackOnMidiInput (true);
  settings.set_panLaw (2);
  settings.set_panAlgorithm (0);
  settings.set_undoStackLength (512);
  settings.set_autoSelectTracks (false);
  settings.set_openPluginsOnInstantiation (false);
  settings.set_showCacheActivity (true);
  settings.set_lv2SearchPaths ({ u"/tmp/lv2"_s });
  settings.set_vst3SearchPaths ({ u"/tmp/vst3"_s });
  settings.set_clapSearchPaths ({ u"/tmp/clap"_s });
  settings.set_auSearchPaths ({ u"/tmp/au"_s });
  settings.set_recentProjects ({ u"/tmp/recent.zpj"_s });

  settings.reset_preference_settings ();

  EXPECT_EQ (settings.themeMode (), AppSettings::ThemeMode::Dark);
  EXPECT_EQ (settings.accentToken (), u"zrythm-orange"_s);
  EXPECT_EQ (settings.uiLocale (), QString ());
  EXPECT_TRUE (settings.transportReturnToCue ());
  EXPECT_FALSE (settings.startPlaybackOnMidiInput ());
  // Pan law/algorithm and automatic track selection are not part of the
  // dialog's preferences: the reset must preserve them.
  EXPECT_EQ (settings.panLaw (), 2);
  EXPECT_EQ (settings.panAlgorithm (), 0);
  EXPECT_FALSE (settings.autoSelectTracks ());
  EXPECT_EQ (settings.undoStackLength (), 128);
  EXPECT_TRUE (settings.openPluginsOnInstantiation ());
  EXPECT_FALSE (settings.showCacheActivity ());
  EXPECT_TRUE (settings.lv2SearchPaths ().isEmpty ());
  EXPECT_TRUE (settings.vst3SearchPaths ().isEmpty ());
  EXPECT_TRUE (settings.clapSearchPaths ().isEmpty ());
  EXPECT_TRUE (settings.auSearchPaths ().isEmpty ());
  EXPECT_EQ (settings.recentProjects (), QStringList ({ u"/tmp/recent.zpj"_s }));
  EXPECT_EQ (
    backend_ptr->value (QAnyStringView ("recentProjects"), {}),
    QVariant (QStringList ({ u"/tmp/recent.zpj"_s })));
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
