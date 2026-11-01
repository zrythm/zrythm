// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "plugins/plugin_catalog.h"
#include "utils/exceptions.h"

#include <QSignalSpy>

#include "helpers/scoped_qcoreapplication.h"

#include <gtest/gtest.h>

using namespace zrythm::test_helpers;
using namespace Qt::StringLiterals;

namespace zrythm::plugins::discovery
{

class PluginCatalogTest : public ::testing::Test, private ScopedQCoreApplication
{
protected:
  std::unique_ptr<PluginDescriptor> make_descriptor (
    const QString         &name,
    int                    unique_id,
    Protocol::ProtocolType protocol = Protocol::ProtocolType::VST3,
    const QString         &identifier = QStringLiteral ("/plugins/test.vst3"))
  {
    auto descriptor = std::make_unique<PluginDescriptor> ();
    descriptor->name_ = utils::Utf8String::from_qstring (name);
    descriptor->author_ = u8"Zrythm";
    descriptor->descriptive_name_ = u8"A test plugin";
    descriptor->version_ = u8"1.2.3";
    descriptor->protocol_ = protocol;
    descriptor->path_or_id_ =
      utils::Utf8String::from_qstring (identifier).to_path ();
    descriptor->unique_id_ = unique_id;
    descriptor->juce_compat_deprecated_unique_id_ = unique_id;
    descriptor->num_audio_ins_ = 2;
    descriptor->num_audio_outs_ = 2;
    descriptor->num_midi_ins_ = 1;
    descriptor->num_midi_outs_ = 1;
    descriptor->num_ctrl_ins_ = 3;
    descriptor->num_ctrl_outs_ = 4;
    descriptor->num_cv_ins_ = 5;
    descriptor->num_cv_outs_ = 6;
    descriptor->last_file_mod_time_ms_ = 1000;
    descriptor->last_info_update_time_ms_ = 2000;
    return descriptor;
  }

  PluginDescriptor * descriptor_at (const PluginCatalog &catalog, int row)
  {
    const auto data =
      catalog.data (catalog.index (row, 0), PluginCatalog::DescriptorRole);
    return qobject_cast<PluginDescriptor *> (data.value<QObject *> ());
  }

  std::shared_ptr<PluginCatalog> catalog_ = std::make_shared<PluginCatalog> ();
};

TEST_F (PluginCatalogTest, ModelExposesDescriptorRoles)
{
  ASSERT_EQ (catalog_->rowCount (), 0);
  ASSERT_EQ (catalog_->columnCount (), 1);
  ASSERT_FALSE (catalog_->index (0, 0).isValid ());
  EXPECT_FALSE (
    catalog_->data (catalog_->index (0, 0), PluginCatalog::DescriptorNameRole)
      .isValid ());

  const auto roles = catalog_->roleNames ();
  EXPECT_EQ (
    roles[PluginCatalog::DescriptorRole], QByteArrayLiteral ("descriptor"));
  EXPECT_EQ (
    roles[PluginCatalog::DescriptorNameRole], QByteArrayLiteral ("name"));

  ASSERT_TRUE (catalog_->add_descriptor (make_descriptor (u"Test"_s, 1)));
  ASSERT_EQ (catalog_->rowCount (), 1);

  const auto name =
    catalog_->data (catalog_->index (0, 0), PluginCatalog::DescriptorNameRole);
  EXPECT_EQ (name.toString (), u"Test"_s);
  EXPECT_NE (descriptor_at (*catalog_, 0), nullptr);
  EXPECT_FALSE (
    catalog_->data (catalog_->index (0, 0), Qt::UserRole + 100).isValid ());
}

TEST_F (PluginCatalogTest, DuplicateIdentityIsUpdatedInPlace)
{
  QSignalSpy rows_inserted (catalog_.get (), &QAbstractItemModel::rowsInserted);
  QSignalSpy rows_removed (catalog_.get (), &QAbstractItemModel::rowsRemoved);

  ASSERT_TRUE (catalog_->add_descriptor (make_descriptor (u"First"_s, 1)));

  auto changed = make_descriptor (u"Changed"_s, 1);
  changed->num_midi_outs_ = 0;
  ASSERT_FALSE (catalog_->add_descriptor (std::move (changed)));
  ASSERT_EQ (catalog_->rowCount (), 1);
  const auto * updated_descriptor = descriptor_at (*catalog_, 0);
  ASSERT_NE (updated_descriptor, nullptr);
  EXPECT_EQ (updated_descriptor->name_.to_qstring (), u"Changed"_s);
  EXPECT_EQ (updated_descriptor->num_midi_outs_, 0);
  EXPECT_EQ (rows_inserted.count (), 1);
  EXPECT_EQ (rows_removed.count (), 0);
}

TEST_F (PluginCatalogTest, ScanReplacementUsesRowLevelNotifications)
{
  ASSERT_TRUE (catalog_->add_descriptor (
    make_descriptor (u"A1"_s, 1, Protocol::ProtocolType::VST3, u"/a.vst3"_s)));
  ASSERT_TRUE (catalog_->add_descriptor (
    make_descriptor (u"B"_s, 2, Protocol::ProtocolType::VST3, u"/b.vst3"_s)));
  ASSERT_TRUE (catalog_->add_descriptor (
    make_descriptor (u"A2"_s, 3, Protocol::ProtocolType::VST3, u"/a.vst3"_s)));
  ASSERT_EQ (catalog_->rowCount (), 3);

  QSignalSpy rows_removed (catalog_.get (), &QAbstractItemModel::rowsRemoved);
  QSignalSpy rows_inserted (catalog_.get (), &QAbstractItemModel::rowsInserted);
  QSignalSpy data_changed (catalog_.get (), &QAbstractItemModel::dataChanged);

  std::vector<std::unique_ptr<PluginDescriptor>> replacement;
  auto                                           changed_a1 = make_descriptor (
    u"A1 Changed"_s, 1, Protocol::ProtocolType::VST3, u"/a.vst3"_s);
  changed_a1->num_midi_outs_ = 0;
  replacement.push_back (std::move (changed_a1));
  replacement.push_back (
    make_descriptor (u"A3"_s, 4, Protocol::ProtocolType::VST3, u"/a.vst3"_s));
  ASSERT_TRUE (catalog_->replace_results_for_scan (
    Protocol::ProtocolType::VST3,
    utils::Utf8String::from_qstring (u"/a.vst3"_s), std::move (replacement)));

  ASSERT_EQ (rows_removed.count (), 1);
  EXPECT_EQ (rows_removed.first ().at (1).toInt (), 2);
  EXPECT_EQ (rows_removed.first ().at (2).toInt (), 2);
  EXPECT_EQ (rows_inserted.count (), 1);
  EXPECT_EQ (data_changed.count (), 1);
  ASSERT_EQ (catalog_->rowCount (), 3);

  QStringList names;
  for (int row = 0; row < catalog_->rowCount (); ++row)
    names.push_back (descriptor_at (*catalog_, row)->name_.to_qstring ());
  EXPECT_EQ (names, (QStringList{ u"A1 Changed"_s, u"B"_s, u"A3"_s }));
}

TEST_F (PluginCatalogTest, SuccessfulEmptyScanRemovesStaleRows)
{
  const auto identifier = u"/empty.lv2"_s;
  ASSERT_TRUE (catalog_->add_descriptor (
    make_descriptor (u"LV2"_s, 1, Protocol::ProtocolType::LV2, identifier)));

  QSignalSpy rows_removed (catalog_.get (), &QAbstractItemModel::rowsRemoved);
  ASSERT_TRUE (catalog_->replace_results_for_scan (
    Protocol::ProtocolType::LV2, utils::Utf8String::from_qstring (identifier),
    {}));
  ASSERT_EQ (rows_removed.count (), 1);
  EXPECT_EQ (catalog_->rowCount (), 0);
}

TEST_F (PluginCatalogTest, ScanReplacementOnlyAffectsScannedProtocol)
{
  const auto identifier = u"/shared-plugin"_s;
  ASSERT_TRUE (catalog_->add_descriptor (
    make_descriptor (u"VST3"_s, 1, Protocol::ProtocolType::VST3, identifier)));
  ASSERT_TRUE (catalog_->add_descriptor (
    make_descriptor (u"CLAP"_s, 2, Protocol::ProtocolType::CLAP, identifier)));

  ASSERT_TRUE (catalog_->replace_results_for_scan (
    Protocol::ProtocolType::VST3, utils::Utf8String::from_qstring (identifier),
    {}));

  EXPECT_EQ (catalog_->rowCount (), 1);
  EXPECT_EQ (
    descriptor_at (*catalog_, 0)->protocol_, Protocol::ProtocolType::CLAP);
}

TEST_F (PluginCatalogTest, UnchangedMetadataRefreshesScanTimestamps)
{
  const auto identifier = u"/timestamped.vst3"_s;
  ASSERT_TRUE (catalog_->add_descriptor (make_descriptor (
    u"Unchanged"_s, 1, Protocol::ProtocolType::VST3, identifier)));

  std::vector<std::unique_ptr<PluginDescriptor>> replacement;
  auto                                           rescanned = make_descriptor (
    u"Unchanged"_s, 1, Protocol::ProtocolType::VST3, identifier);
  rescanned->last_file_mod_time_ms_ = 3000;
  rescanned->last_info_update_time_ms_ = 4000;
  replacement.push_back (std::move (rescanned));
  ASSERT_TRUE (catalog_->replace_results_for_scan (
    Protocol::ProtocolType::VST3, utils::Utf8String::from_qstring (identifier),
    std::move (replacement)));

  const auto * stored = descriptor_at (*catalog_, 0);
  ASSERT_NE (stored, nullptr);
  EXPECT_EQ (stored->last_file_mod_time_ms_, 3000);
  EXPECT_EQ (stored->last_info_update_time_ms_, 4000);
}

TEST_F (PluginCatalogTest, FlatIndexesDoNotRetainDescriptorOwnership)
{
  ASSERT_TRUE (catalog_->add_descriptor (make_descriptor (u"First"_s, 1)));
  const QPersistentModelIndex persistent_index (catalog_->index (0, 0));

  auto changed = make_descriptor (u"Changed"_s, 1);
  ASSERT_FALSE (catalog_->add_descriptor (std::move (changed)));

  ASSERT_TRUE (persistent_index.isValid ());
  EXPECT_EQ (persistent_index.internalPointer (), nullptr);
  EXPECT_EQ (
    persistent_index.data (PluginCatalog::DescriptorNameRole).toString (),
    u"Changed"_s);
}

TEST_F (PluginCatalogTest, AvailableFormatsChangeOnlyWhenSetChanges)
{
  QSignalSpy formats_changed (
    catalog_.get (), &PluginCatalog::availableFormatsChanged);

  catalog_->add_descriptor (
    make_descriptor (u"VST3"_s, 1, Protocol::ProtocolType::VST3));
  QTRY_COMPARE_WITH_TIMEOUT (
    catalog_->availableFormats (), QStringList{ u"VST3"_s }, 1000);

  catalog_->add_descriptor (
    make_descriptor (u"VST3 Second"_s, 2, Protocol::ProtocolType::VST3));
  catalog_->add_descriptor (make_descriptor (
    u"CLAP"_s, 3, Protocol::ProtocolType::CLAP, u"/test.clap"_s));
  QTRY_COMPARE_WITH_TIMEOUT (
    catalog_->availableFormats (), (QStringList{ u"CLAP"_s, u"VST3"_s }), 1000);
  QTRY_COMPARE_WITH_TIMEOUT (formats_changed.count (), 2, 1000);
}

TEST_F (PluginCatalogTest, ClearingCatalogClearsAvailableFormats)
{
  QSignalSpy formats_changed (
    catalog_.get (), &PluginCatalog::availableFormatsChanged);
  ASSERT_TRUE (catalog_->add_descriptor (make_descriptor (u"VST3"_s, 1)));
  QTRY_COMPARE_WITH_TIMEOUT (
    catalog_->availableFormats (), QStringList{ u"VST3"_s }, 1000);
  QCoreApplication::sendPostedEvents (nullptr, QEvent::MetaCall);
  EXPECT_EQ (formats_changed.count (), 1);

  catalog_->clear ();

  QCoreApplication::sendPostedEvents (nullptr, QEvent::MetaCall);
  EXPECT_EQ (formats_changed.count (), 2);
  EXPECT_TRUE (catalog_->availableFormats ().isEmpty ());
}

TEST_F (PluginCatalogTest, ScanStateResolvesBlacklistAndRescan)
{
  const utils::Utf8String identifier =
    utils::Utf8String::from_qstring (u"/test.vst3"_s);
  const auto descriptor = make_descriptor (
    u"Test"_s, 1, Protocol::ProtocolType::VST3, u"/test.vst3"_s);
  ASSERT_TRUE (catalog_->add_descriptor (utils::clone_unique (*descriptor)));

  EXPECT_EQ (
    catalog_->scan_state (
      Protocol::ProtocolType::VST3, identifier,
      [] (const PluginDescriptor &) { return false; }),
    PluginCatalog::ScanState::UpToDate);
  EXPECT_EQ (
    catalog_->scan_state (
      Protocol::ProtocolType::VST3, identifier,
      [] (const PluginDescriptor &) { return true; }),
    PluginCatalog::ScanState::NeedsScan);
  EXPECT_EQ (
    catalog_->scan_state (
      Protocol::ProtocolType::LV2, identifier,
      [] (const PluginDescriptor &) { return false; }),
    PluginCatalog::ScanState::NeedsScan);

  catalog_->blacklist_identifier (Protocol::ProtocolType::VST3, identifier);
  EXPECT_TRUE (
    catalog_->is_blacklisted (Protocol::ProtocolType::VST3, identifier));
  EXPECT_EQ (
    catalog_->scan_state (
      Protocol::ProtocolType::VST3, identifier,
      [] (const PluginDescriptor &) { return false; }),
    PluginCatalog::ScanState::Blacklisted);
}

TEST_F (PluginCatalogTest, ScanReplacementRejectsDuplicateIdentities)
{
  const auto identifier = u"/duplicates.vst3"_s;
  ASSERT_TRUE (catalog_->add_descriptor (make_descriptor (
    u"Existing"_s, 1, Protocol::ProtocolType::VST3, identifier)));

  std::vector<std::unique_ptr<PluginDescriptor>> results;
  results.push_back (
    make_descriptor (u"First"_s, 2, Protocol::ProtocolType::VST3, identifier));
  results.push_back (
    make_descriptor (u"Second"_s, 2, Protocol::ProtocolType::VST3, identifier));

  const auto replacement = catalog_->replace_results_for_scan (
    Protocol::ProtocolType::VST3, utils::Utf8String::from_qstring (identifier),
    std::move (results));
  EXPECT_FALSE (replacement.has_value ());
  ASSERT_EQ (catalog_->rowCount (), 1);
  EXPECT_EQ (descriptor_at (*catalog_, 0)->name_.to_qstring (), u"Existing"_s);
}

TEST_F (PluginCatalogTest, SerializationRoundTrip)
{
  auto original = make_descriptor (u"Test"_s, 42);
  original->num_midi_outs_ = 2;
  original->num_cv_outs_ = 3;
  ASSERT_TRUE (catalog_->add_descriptor (std::move (original)));
  const auto broken_identifier =
    utils::Utf8String::from_qstring (u"/broken.clap"_s);
  catalog_->blacklist_identifier (
    Protocol::ProtocolType::CLAP, broken_identifier);

  PluginCatalog restored;
  restored.deserialize (catalog_->serialize ());
  ASSERT_EQ (restored.rowCount (), 1);

  auto * restored_descriptor = descriptor_at (restored, 0);
  ASSERT_NE (restored_descriptor, nullptr);
  EXPECT_EQ (restored_descriptor->name_.to_qstring (), u"Test"_s);
  EXPECT_EQ (restored_descriptor->unique_id_, 42);
  EXPECT_EQ (restored_descriptor->num_midi_outs_, 2);
  EXPECT_EQ (restored_descriptor->num_cv_outs_, 3);
  EXPECT_EQ (
    restored_descriptor->last_file_mod_time_ms_,
    descriptor_at (*catalog_, 0)->last_file_mod_time_ms_);
  EXPECT_TRUE (
    restored.is_blacklisted (Protocol::ProtocolType::CLAP, broken_identifier));
}

TEST_F (PluginCatalogTest, SerializationRoundTripAcceptsLargePortCounts)
{
  auto original = make_descriptor (u"Many Ports"_s, 43);
  original->num_ctrl_ins_ = 2617;
  ASSERT_TRUE (catalog_->add_descriptor (std::move (original)));

  PluginCatalog restored;
  restored.deserialize (catalog_->serialize ());
  ASSERT_EQ (restored.rowCount (), 1);
  EXPECT_EQ (descriptor_at (restored, 0)->num_ctrl_ins_, 2617);
}

TEST_F (PluginCatalogTest, DeserializeRejectsWrongDocumentType)
{
  auto json = catalog_->serialize ();
  json[utils::serialization::kDocumentTypeKey] = "NotKnownPlugins";
  EXPECT_THROW (catalog_->deserialize (json), utils::ZrythmException);
}

} // namespace zrythm::plugins::discovery
