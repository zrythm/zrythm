// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <atomic>
#include <fstream>
#include <future>
#include <stdexcept>

#include "plugins/plugin_format_utils.h"
#include "plugins/plugin_scan_manager.h"
#include "utils/icloneable.h"
#include "utils/io_utils.h"
#include "utils/logger.h"

#include <QSignalSpy>

#include "helpers/scoped_juce_qapplication.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace zrythm::test_helpers;

namespace zrythm::plugins
{

namespace
{

class TestScanner final : public discovery::PluginScanner
{
public:
  discovery::PluginScanResult scan (
    const utils::Utf8String &format_name,
    const utils::Utf8String &identifier,
    const std::function<bool ()> &) override
  {
    ++call_count;
    scanned_identifiers.push_back (identifier);
    if (throw_exception)
      throw std::runtime_error ("requested scanner exception");
    if (fail)
      return std::unexpected (
        utils::Utf8String::from_utf8_encoded_string ("requested scan failure"));
    if (block_next_scan.exchange (false))
      {
        scan_started.set_value ();
        scan_release_future.wait ();
      }

    auto descriptor = std::make_unique<PluginDescriptor> ();
    descriptor->name_ = u8"Test Plugin";
    descriptor->protocol_ = Protocol::ProtocolType::VST3;
    descriptor->path_or_id_ = identifier.to_path ();
    descriptor->unique_id_ = 12345;
    descriptor->juce_compat_deprecated_unique_id_ = 12345;
    descriptor->last_file_mod_time_ms_ = file_modification_ms.value_or (0);
    descriptor->last_info_update_time_ms_ = 2000;
    descriptor->num_audio_ins_ = 2;
    descriptor->num_audio_outs_ = 2;
    descriptor->num_midi_outs_ = 1;

    std::vector<std::unique_ptr<PluginDescriptor>> descriptors;
    if (return_descriptor)
      {
        descriptors.push_back (std::move (descriptor));
        if (duplicate_descriptors)
          descriptors.push_back (utils::clone_unique (*descriptors.front ()));
      }
    return descriptors;
  }

  bool                     fail = false;
  bool                     return_descriptor = true;
  bool                     duplicate_descriptors = false;
  bool                     throw_exception = false;
  std::atomic<bool>        block_next_scan{ false };
  std::promise<void>       scan_started;
  std::promise<void>       scan_release;
  std::shared_future<void> scan_started_future =
    scan_started.get_future ().share ();
  std::shared_future<void> scan_release_future =
    scan_release.get_future ().share ();
  std::optional<std::int64_t>    file_modification_ms;
  int                            call_count = 0;
  std::vector<utils::Utf8String> scanned_identifiers;
};

class TempVst3Path
{
public:
  explicit TempVst3Path (const QString &test_name)
      : dir_ (utils::io::make_tmp_dir (test_name + "_XXXXXX"))
  {
    const auto dir_path =
      utils::Utf8String::from_qstring (dir_->path ()).to_path ();
    plugin_path_ = dir_path / "test-plugin.vst3";
    std::ofstream file (plugin_path_);
    file << "placeholder";
  }

  utils::Utf8String identifier () const
  {
    return utils::Utf8String::from_path (plugin_path_);
  }

  std::filesystem::path search_path () const
  {
    return plugin_path_.parent_path ();
  }

  // Rescan detection compares juce Unix-epoch milliseconds, so the test
  // reads the timestamp through the same utility
  std::int64_t modification_ms () const
  {
    return zrythm::plugins::effective_modification_time (
             utils::Utf8String::from_path (plugin_path_).to_juce_file ())
      .toMilliseconds ();
  }

private:
  std::unique_ptr<QTemporaryDir> dir_;
  std::filesystem::path          plugin_path_;
};

struct ScanManagerTest : ::testing::Test, private ScopedJuceQApplication
{
  std::shared_ptr<discovery::PluginCatalog> catalog =
    std::make_shared<discovery::PluginCatalog> ();
  std::shared_ptr<PluginScanFormatManager> formats =
    std::make_shared<PluginScanFormatManager> (nullptr);

  ProtocolPluginPathsProvider
  paths_for (const std::filesystem::path &search_path)
  {
    return [search_path] (Protocol::ProtocolType protocol) {
      auto paths = std::make_unique<utils::FilePathList> ();
      if (protocol == Protocol::ProtocolType::VST3)
        paths->add_path (search_path);
      return paths;
    };
  }
};

} // namespace

TEST_F (ScanManagerTest, SuccessfulScanAddsDescriptor)
{
  TempVst3Path temp_path ("plugin_scan_success");
  auto         scanner = std::make_unique<TestScanner> ();
  scanner->file_modification_ms = temp_path.modification_ms ();
  TestScanner * scanner_ptr = scanner.get ();

  PluginScanManager manager (
    catalog, formats, paths_for (temp_path.search_path ()), std::move (scanner));

  QSignalSpy finished_spy (&manager, &PluginScanManager::scanningFinished);
  manager.beginScan ();
  ASSERT_TRUE (finished_spy.wait (5000));

  EXPECT_EQ (scanner_ptr->call_count, 1);
  ASSERT_EQ (catalog->rowCount (), 1);
  EXPECT_EQ (
    catalog
      ->data (catalog->index (0, 0), discovery::PluginCatalog::DescriptorNameRole)
      .toString (),
    QString ("Test Plugin"));
}

TEST_F (ScanManagerTest, FailedScanIsBlacklisted)
{
  TempVst3Path temp_path ("plugin_scan_failure");
  auto         scanner = std::make_unique<TestScanner> ();
  scanner->fail = true;
  TestScanner * scanner_ptr = scanner.get ();

  PluginScanManager manager (
    catalog, formats, paths_for (temp_path.search_path ()), std::move (scanner));

  QSignalSpy finished_spy (&manager, &PluginScanManager::scanningFinished);
  manager.beginScan ();
  ASSERT_TRUE (finished_spy.wait (5000));

  EXPECT_EQ (scanner_ptr->call_count, 1);
  EXPECT_EQ (catalog->rowCount (), 0);
  EXPECT_TRUE (catalog->is_blacklisted (
    Protocol::ProtocolType::VST3, temp_path.identifier ()));
}

TEST_F (ScanManagerTest, EmptySuccessfulScanIsNotBlacklisted)
{
  TempVst3Path temp_path ("plugin_scan_empty");
  auto         scanner = std::make_unique<TestScanner> ();
  scanner->return_descriptor = false;

  PluginScanManager manager (
    catalog, formats, paths_for (temp_path.search_path ()), std::move (scanner));

  QSignalSpy finished_spy (&manager, &PluginScanManager::scanningFinished);
  manager.beginScan ();
  ASSERT_TRUE (finished_spy.wait (5000));

  EXPECT_EQ (catalog->rowCount (), 0);
  EXPECT_FALSE (catalog->is_blacklisted (
    Protocol::ProtocolType::VST3, temp_path.identifier ()));
}

TEST_F (ScanManagerTest, UnchangedIdentifierIsNotScannedAgain)
{
  TempVst3Path temp_path ("plugin_scan_unchanged");
  auto         scanner = std::make_unique<TestScanner> ();
  scanner->file_modification_ms = temp_path.modification_ms ();
  TestScanner * scanner_ptr = scanner.get ();

  PluginScanManager manager (
    catalog, formats, paths_for (temp_path.search_path ()), std::move (scanner));

  QSignalSpy first_finished (&manager, &PluginScanManager::scanningFinished);
  manager.beginScan ();
  ASSERT_TRUE (first_finished.wait (5000));
  ASSERT_EQ (catalog->rowCount (), 1);

  QSignalSpy second_finished (&manager, &PluginScanManager::scanningFinished);
  manager.beginScan ();
  ASSERT_TRUE (second_finished.wait (5000));
  EXPECT_EQ (scanner_ptr->call_count, 1);
  EXPECT_EQ (catalog->rowCount (), 1);
}

TEST_F (ScanManagerTest, RestartDuringActiveScanRunsNewScan)
{
  TempVst3Path temp_path ("plugin_scan_restart");
  auto         scanner = std::make_unique<TestScanner> ();
  scanner->block_next_scan = true;
  scanner->file_modification_ms = temp_path.modification_ms () - 1000;
  TestScanner * scanner_ptr = scanner.get ();

  PluginScanManager manager (
    catalog, formats, paths_for (temp_path.search_path ()), std::move (scanner));

  QSignalSpy finished_spy (&manager, &PluginScanManager::scanningFinished);
  manager.beginScan ();
  scanner_ptr->scan_started_future.wait ();

  manager.beginScan ();
  scanner_ptr->scan_release.set_value ();

  ASSERT_TRUE (finished_spy.wait (5000));
  EXPECT_EQ (finished_spy.count (), 1);
  EXPECT_EQ (scanner_ptr->call_count, 2);
  ASSERT_EQ (catalog->rowCount (), 1);
}

TEST_F (ScanManagerTest, DestructionDuringActiveScanStopsWorker)
{
  TempVst3Path temp_path ("plugin_scan_destruction");
  auto         scanner = std::make_unique<TestScanner> ();
  scanner->block_next_scan = true;
  scanner->file_modification_ms = temp_path.modification_ms ();
  TestScanner * scanner_ptr = scanner.get ();

  auto manager = std::make_unique<PluginScanManager> (
    catalog, formats, paths_for (temp_path.search_path ()), std::move (scanner));
  manager->beginScan ();
  scanner_ptr->scan_started_future.wait ();

  scanner_ptr->scan_release.set_value ();
  manager.reset ();
  EXPECT_EQ (catalog->rowCount (), 0);
}

// A scan result containing two descriptors with the same identity is rejected
// and the identifier is blacklisted, like any other failed scan
TEST_F (ScanManagerTest, DuplicateScanResultsAreBlacklisted)
{
  TempVst3Path temp_path ("plugin_scan_duplicates");
  auto         scanner = std::make_unique<TestScanner> ();
  scanner->duplicate_descriptors = true;
  scanner->file_modification_ms = temp_path.modification_ms ();

  PluginScanManager manager (
    catalog, formats, paths_for (temp_path.search_path ()), std::move (scanner));

  QSignalSpy finished_spy (&manager, &PluginScanManager::scanningFinished);
  manager.beginScan ();
  ASSERT_TRUE (finished_spy.wait (5000));

  EXPECT_EQ (catalog->rowCount (), 0);
  EXPECT_TRUE (catalog->is_blacklisted (
    Protocol::ProtocolType::VST3, temp_path.identifier ()));
}

// A scan result obtained while a stop was requested is discarded: it is
// neither applied nor recorded as a failure
TEST_F (ScanManagerTest, StopDuringScanDiscardsResult)
{
  TempVst3Path temp_path ("plugin_scan_stop_discard");
  auto         scanner = std::make_unique<TestScanner> ();
  scanner->block_next_scan = true;
  scanner->file_modification_ms = temp_path.modification_ms ();
  TestScanner * scanner_ptr = scanner.get ();

  PluginScanManager manager (
    catalog, formats, paths_for (temp_path.search_path ()), std::move (scanner));

  QSignalSpy finished_spy (&manager, &PluginScanManager::scanningFinished);
  manager.beginScan ();
  scanner_ptr->scan_started_future.wait ();

  manager.requestStop ();
  scanner_ptr->scan_release.set_value ();

  ASSERT_TRUE (finished_spy.wait (5000));
  EXPECT_EQ (catalog->rowCount (), 0);
  EXPECT_FALSE (catalog->is_blacklisted (
    Protocol::ProtocolType::VST3, temp_path.identifier ()));
}

TEST_F (ScanManagerTest, ScannerExceptionStillReportsCompletion)
{
  TempVst3Path temp_path ("plugin_scan_exception");
  auto         scanner = std::make_unique<TestScanner> ();
  scanner->throw_exception = true;

  PluginScanManager manager (
    catalog, formats, paths_for (temp_path.search_path ()), std::move (scanner));

  QSignalSpy finished_spy (&manager, &PluginScanManager::scanningFinished);
  manager.beginScan ();
  ASSERT_TRUE (finished_spy.wait (5000));

  EXPECT_EQ (catalog->rowCount (), 0);
  EXPECT_FALSE (catalog->is_blacklisted (
    Protocol::ProtocolType::VST3, temp_path.identifier ()));
}

} // namespace zrythm::plugins
