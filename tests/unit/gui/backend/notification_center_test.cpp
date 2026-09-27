// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <thread>

#include "gui/backend/notification_center.h"
#include "gui/backend/notification_model.h"
#include "utils/logger.h"

#include <QSignalSpy>
#include <QTest>

#include "helpers/scoped_qcoreapplication.h"

#include <gtest/gtest.h>

using namespace Qt::StringLiterals;

namespace zrythm::gui
{

class NotificationCenterTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    app_ = std::make_unique<test_helpers::ScopedQCoreApplication> ();
    center_ = std::make_unique<NotificationCenter> ();
  }

  void TearDown () override
  {
    center_.reset ();
    app_.reset ();
  }

  std::unique_ptr<test_helpers::ScopedQCoreApplication> app_;
  std::unique_ptr<NotificationCenter>                   center_;
};

TEST_F (NotificationCenterTest, GuiThreadPostEmitsAndStoresInHistory)
{
  QSignalSpy spy (center_.get (), &NotificationCenter::notificationPosted);

  center_->postError (u"Cannot Perform Operation"_s, u"Paste refused"_s);

  EXPECT_EQ (spy.count (), 1);
  ASSERT_EQ (center_->history ()->rowCount (), 1);

  const auto     entries = center_->history ()->entries ();
  Notification * stored = entries[0];
  EXPECT_EQ (stored->severity (), Notification::Error);
  EXPECT_EQ (stored->title (), u"Cannot Perform Operation"_s);
  EXPECT_EQ (stored->detail (), u"Paste refused"_s);
  EXPECT_FALSE (stored->isAcknowledged ());
  EXPECT_FALSE (stored->timestamp ().isNull ());
}

TEST_F (NotificationCenterTest, PostFromWorkerThreadDeliversOnGuiThread)
{
  QSignalSpy spy (center_.get (), &NotificationCenter::notificationPosted);

  std::jthread worker ([this] {
    center_->postWarning (u"Device Disconnected"_s, u"Interface gone"_s);
  });

  EXPECT_TRUE (spy.wait (1000));
  EXPECT_EQ (spy.count (), 1);
  EXPECT_EQ (
    center_->history ()->entries ()[0]->title (), u"Device Disconnected"_s);
}

TEST_F (NotificationCenterTest, HistoryStoresEachOccurrenceAsItsOwnRow)
{
  center_->post (Notification::Error, u"Failed"_s, {}, u"plugin:1"_s);
  center_->post (Notification::Info, u"Other"_s, {}, u"plugin:1"_s);
  center_->post (Notification::Error, u"Failed"_s, {}, u"plugin:1"_s);
  center_->post (Notification::Error, u"Failed"_s, {}, u"plugin:2"_s);

  // Four posts, four rows, newest first
  ASSERT_EQ (center_->history ()->rowCount (), 4);
  const auto entries = center_->history ()->entries ();
  EXPECT_EQ (entries[0]->contextTag (), u"plugin:2"_s);
  EXPECT_EQ (entries[1]->contextTag (), u"plugin:1"_s);
  EXPECT_EQ (entries[1]->severity (), Notification::Error);
  EXPECT_EQ (entries[2]->contextTag (), u"plugin:1"_s);
  EXPECT_EQ (entries[2]->severity (), Notification::Info);
}

TEST_F (NotificationCenterTest, HistoryCappedAtMaxNotifications)
{
  for (int i = 0; i < NotificationModel::MAX_NOTIFICATIONS + 50; ++i)
    {
      center_->postInfo (u"Info %1"_s.arg (i), {});
    }

  EXPECT_EQ (
    center_->history ()->rowCount (), NotificationModel::MAX_NOTIFICATIONS);
  EXPECT_EQ (
    center_->history ()->entries ()[0]->title (),
    u"Info %1"_s.arg (NotificationModel::MAX_NOTIFICATIONS + 49));
}

TEST_F (NotificationCenterTest, PostsAreLoggedWithSeverityName)
{
  utils::init_logging (utils::LoggerType::Test);

  center_->postInfo (u"Info Occurrence"_s, {});
  center_->postError (u"Error Occurrence"_s, u"extra detail"_s);

  const auto entries = utils::get_last_log_entries (10);
  const auto contains = [&entries] (std::string_view needle) {
    return std::ranges::any_of (entries, [needle] (const auto &entry) {
      return entry.str ().find (needle) != std::string::npos;
    });
  };
  EXPECT_TRUE (contains ("[notification] [Info] Info Occurrence"));
  EXPECT_TRUE (
    contains ("[notification] [Error] Error Occurrence — extra detail"));
}

TEST_F (NotificationCenterTest, RecurringOccurrenceIsSeparateAndUnacknowledged)
{
  center_->postError (u"Recurring"_s, {});

  center_->history ()->set_all_acknowledged ();
  ASSERT_TRUE (center_->history ()->entries ()[0]->isAcknowledged ());

  center_->postError (u"Recurring"_s, {});

  ASSERT_EQ (center_->history ()->rowCount (), 2);
  EXPECT_FALSE (center_->history ()->entries ()[0]->isAcknowledged ());
  EXPECT_TRUE (center_->history ()->entries ()[1]->isAcknowledged ());
}

TEST_F (NotificationCenterTest, UnacknowledgedCriticalsListOnlyUnpresented)
{
  center_->postInfo (u"Autosaved"_s, {});
  center_->postError (u"Cannot Perform Operation"_s, {});
  center_->postCritical (u"Audio Device Initialization Failed"_s, {});

  const auto unpresented = center_->unacknowledgedCriticals ();
  ASSERT_EQ (unpresented.size (), 1);
  EXPECT_EQ (unpresented[0]->title (), u"Audio Device Initialization Failed"_s);

  unpresented[0]->setAcknowledged (true);
  EXPECT_TRUE (center_->history ()->entries ()[0]->isAcknowledged ());
  EXPECT_TRUE (center_->unacknowledgedCriticals ().isEmpty ());
}

TEST_F (NotificationCenterTest, AcknowledgedCriticalIsListedAgainOnRecurring)
{
  center_->postCritical (u"Project Loading Failed"_s, {});
  center_->unacknowledgedCriticals ()[0]->setAcknowledged (true);
  ASSERT_TRUE (center_->unacknowledgedCriticals ().isEmpty ());

  center_->postCritical (u"Project Loading Failed"_s, {});

  EXPECT_EQ (center_->unacknowledgedCriticals ().size (), 1);
}

TEST_F (
  NotificationCenterTest,
  UnacknowledgedAttentionCountCountsWarningsAndErrorsOnly)
{
  EXPECT_EQ (center_->history ()->unacknowledgedAttentionCount (), 0);
  EXPECT_EQ (
    center_->history ()->highestUnacknowledgedAttentionSeverity (),
    Notification::Info);

  center_->postInfo (u"Autosaved"_s, {});
  center_->postSuccess (u"Exported"_s, {});
  center_->postWarning (u"Plugin Failed to Load"_s, {});
  EXPECT_EQ (center_->history ()->unacknowledgedAttentionCount (), 1);
  EXPECT_EQ (
    center_->history ()->highestUnacknowledgedAttentionSeverity (),
    Notification::Warning);

  center_->postError (u"Cannot Perform Operation"_s, {});
  center_->postCritical (u"Project Loading Failed"_s, {});
  EXPECT_EQ (center_->history ()->unacknowledgedAttentionCount (), 2);
  EXPECT_EQ (
    center_->history ()->highestUnacknowledgedAttentionSeverity (),
    Notification::Error);

  center_->acknowledgeAll ();
  EXPECT_EQ (center_->history ()->unacknowledgedAttentionCount (), 0);
  EXPECT_EQ (
    center_->history ()->highestUnacknowledgedAttentionSeverity (),
    Notification::Info);
}

TEST_F (NotificationCenterTest, AttentionCountCountsEachOccurrence)
{
  center_->post (Notification::Error, u"Device Disconnected"_s, {}, u"device"_s);
  center_->post (Notification::Error, u"Device Disconnected"_s, {}, u"device"_s);
  center_->post (Notification::Error, u"Device Disconnected"_s, {}, u"device"_s);

  ASSERT_EQ (center_->history ()->rowCount (), 3);
  EXPECT_EQ (center_->history ()->unacknowledgedAttentionCount (), 3);
}

TEST_F (
  NotificationCenterTest,
  AcknowledgementSetOnTheNotificationReachesTheModel)
{
  QSignalSpy data_changed (center_->history (), &NotificationModel::dataChanged);

  center_->postError (u"Cannot Perform Operation"_s, {});
  center_->postError (u"Unplugged"_s, {});
  ASSERT_EQ (data_changed.count (), 0);

  center_->history ()->entries ()[0]->setAcknowledged (true);

  ASSERT_EQ (data_changed.count (), 1);
  EXPECT_EQ (data_changed.at (0).at (0).toModelIndex ().row (), 0);
  EXPECT_EQ (center_->history ()->unacknowledgedAttentionCount (), 1);
}

TEST_F (NotificationCenterTest, ClearHistoryRemovesAllRows)
{
  center_->postError (u"Cannot Perform Operation"_s, {});
  center_->postWarning (u"Plugin Failed to Load"_s, {});

  ASSERT_EQ (center_->history ()->rowCount (), 2);

  center_->clearHistory ();

  EXPECT_EQ (center_->history ()->rowCount (), 0);
  EXPECT_EQ (center_->history ()->unacknowledgedAttentionCount (), 0);
}

} // namespace zrythm::gui
