// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/app_settings.h"
#include "utils/isettings_backend.h"

#include <QSignalSpy>
#include <QStringList>
#include <QVariant>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace ::testing;

namespace zrythm::utils
{

class MockSettingsBackend : public ISettingsBackend
{
public:
  MOCK_METHOD (
    QVariant,
    value,
    (QAnyStringView key, const QVariant &defaultValue),
    (const, override));
  MOCK_METHOD (
    void,
    setValue,
    (QAnyStringView key, const QVariant &value),
    (override));
};

class AppSettingsTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    auto mock_backend = std::make_unique<MockSettingsBackend> ();
    mock_backend_ = mock_backend.get ();
    app_settings_ = std::make_unique<AppSettings> (std::move (mock_backend));
  }

  void TearDown () override { app_settings_.reset (); }

  MockSettingsBackend *        mock_backend_{};
  std::unique_ptr<AppSettings> app_settings_;
};

TEST_F (AppSettingsTest, Constructor)
{
  // Test that AppSettings can be constructed with a mock backend
  EXPECT_NE (app_settings_, nullptr);
}

TEST_F (AppSettingsTest, FirstRunProperty)
{
  // Test default value
  EXPECT_EQ (app_settings_->get_default_first_run (), true);

  // Test getter returns default when backend has no value
  EXPECT_CALL (
    *mock_backend_, value (QAnyStringView ("first_run"), QVariant (true)))
    .WillOnce (Return (QVariant (true)));
  EXPECT_EQ (app_settings_->first_run (), true);

  // Test setter
  EXPECT_CALL (
    *mock_backend_, value (QAnyStringView ("first_run"), QVariant (true)))
    .WillOnce (Return (QVariant (true)));
  EXPECT_CALL (
    *mock_backend_, setValue (QAnyStringView ("first_run"), QVariant (false)))
    .Times (1);

  app_settings_->set_first_run (false);
}

TEST_F (AppSettingsTest, FirstRunSignal)
{
  // Set up mock to return current value
  EXPECT_CALL (
    *mock_backend_, value (QAnyStringView ("first_run"), QVariant (true)))
    .WillOnce (Return (QVariant (true)));

  // Set up mock for setValue
  EXPECT_CALL (
    *mock_backend_, setValue (QAnyStringView ("first_run"), QVariant (false)))
    .Times (1);

  // Connect signal spy
  QSignalSpy spy (app_settings_.get (), &AppSettings::first_runChanged);

  // Change value
  app_settings_->set_first_run (false);

  // Verify signal was emitted
  EXPECT_EQ (spy.count (), 1);
  EXPECT_EQ (spy.at (0).at (0).toBool (), false);
}

TEST_F (AppSettingsTest, FirstRunNoSignalOnSameValue)
{
  // Set up mock to return same value we're setting
  EXPECT_CALL (
    *mock_backend_, value (QAnyStringView ("first_run"), QVariant (true)))
    .WillOnce (Return (QVariant (true)));

  // setValue should not be called since values are equal
  EXPECT_CALL (*mock_backend_, setValue (QAnyStringView ("first_run"), _))
    .Times (0);

  // Connect signal spy
  QSignalSpy spy (app_settings_.get (), &AppSettings::first_runChanged);

  // Set same value
  app_settings_->set_first_run (true);

  // Verify no signal was emitted
  EXPECT_EQ (spy.count (), 0);
}

TEST_F (AppSettingsTest, StringProperty)
{
  // Test icon_theme property
  QString expected_default = u"zrythm-dark"_s;
  EXPECT_EQ (app_settings_->get_default_icon_theme (), expected_default);

  // Test getter
  EXPECT_CALL (
    *mock_backend_,
    value (QAnyStringView ("icon_theme"), QVariant (expected_default)))
    .WillOnce (Return (QVariant (expected_default)));
  EXPECT_EQ (app_settings_->icon_theme (), expected_default);

  // Test setter
  EXPECT_CALL (
    *mock_backend_,
    value (QAnyStringView ("icon_theme"), QVariant (expected_default)))
    .WillOnce (Return (QVariant (expected_default)));
  EXPECT_CALL (
    *mock_backend_,
    setValue (QAnyStringView ("icon_theme"), QVariant (u"light-theme"_s)))
    .Times (1);

  app_settings_->set_icon_theme (u"light-theme"_s);
}

TEST_F (AppSettingsTest, StringListProperty)
{
  // Test recent_projects property
  QStringList expected_default;
  EXPECT_EQ (app_settings_->get_default_recent_projects (), expected_default);

  QStringList test_list = { "project1", "project2", "project3" };

  // Test getter
  EXPECT_CALL (
    *mock_backend_,
    value (QAnyStringView ("recent_projects"), QVariant (expected_default)))
    .WillOnce (Return (QVariant (test_list)));
  EXPECT_EQ (app_settings_->recent_projects (), test_list);

  // Test setter
  EXPECT_CALL (
    *mock_backend_,
    value (QAnyStringView ("recent_projects"), QVariant (expected_default)))
    .WillOnce (Return (QVariant (expected_default)));
  EXPECT_CALL (
    *mock_backend_,
    setValue (QAnyStringView ("recent_projects"), QVariant (test_list)))
    .Times (1);

  app_settings_->set_recent_projects (test_list);
}

TEST_F (AppSettingsTest, NumericProperty)
{
  // Test undoStackLength property
  int expected_default = 128;
  EXPECT_EQ (app_settings_->get_default_undoStackLength (), expected_default);

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
  EXPECT_EQ (app_settings_->get_default_metronomeVolume (), expected_default);

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
    app_settings_->get_default_monitorVolume (), expected_default);

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

}
