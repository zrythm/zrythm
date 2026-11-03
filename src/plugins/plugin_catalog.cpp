// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <ranges>
#include <tuple>
#include <utility>

#include "plugins/plugin_catalog.h"
#include "plugins/plugin_catalog_json_schema.h"
#include "utils/exceptions.h"
#include "utils/icloneable.h"
#include "utils/logger.h"
#include "utils/views.h"

#include <QHash>
#include <QReadLocker>
#include <QWriteLocker>

#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>

namespace zrythm::plugins::discovery
{

namespace
{
constexpr auto kDocumentType = "ZrythmKnownPlugins"sv;
constexpr auto kPluginsKey = "plugins"sv;
constexpr auto kBlacklistedIdentifiersKey = "blacklistedIdentifiers"sv;
constexpr auto kProtocolKey = "protocol"sv;
constexpr auto kIdentifierKey = "identifier"sv;
constexpr int  kMajorSchemaVersion = 1;
constexpr int  kMinorSchemaVersion = 0;

/** Lazily-initialized validator for the embedded known-plugins schema. */
class CatalogSchemaValidator
{
public:
  static CatalogSchemaValidator &get_instance ()
  {
    static CatalogSchemaValidator instance;
    return instance;
  }

  void validate (const nlohmann::json &json) const
  {
    validator_.validate (json);
  }

private:
  CatalogSchemaValidator ()
      : validator_ (
          nlohmann::json::parse (kPluginCatalogSchemaJsonStr),
          nullptr,
          nlohmann::json_schema::default_string_format_check,
          nullptr)
  {
  }

  nlohmann::json_schema::json_validator validator_;
};

QString
format_name (Protocol::ProtocolType protocol)
{
  return utils::Utf8String::
    from_utf8_encoded_string (Protocol::to_string (protocol))
      .to_qstring ();
}
} // namespace

PluginCatalog::PluginCatalog (QObject * parent)
    : QAbstractListModel (parent) { }

PluginCatalog::~PluginCatalog () = default;

QHash<int, QByteArray>
PluginCatalog::roleNames () const
{
  static const QHash<int, QByteArray> roles{
    { DescriptorRole,     "descriptor" },
    { DescriptorNameRole, "name"       },
  };
  return roles;
}

QModelIndex
PluginCatalog::index (int row, int column, const QModelIndex &parent) const
{
  if (parent.isValid ())
    return {};
  QReadLocker lock (&lock_);
  if (row < 0 || row >= static_cast<int> (descriptors_.size ()) || column != 0)
    return {};
  return createIndex (row, column);
}

QModelIndex
PluginCatalog::parent (const QModelIndex &child) const
{
  return {};
}

int
PluginCatalog::rowCount (const QModelIndex &parent) const
{
  return parent.isValid () ? 0 : descriptor_count ();
}

int
PluginCatalog::columnCount (const QModelIndex &parent) const
{
  return parent.isValid () ? 0 : 1;
}

QVariant
PluginCatalog::data (const QModelIndex &index, int role) const
{
  QReadLocker lock (&lock_);
  if (
    index.row () < 0 || index.row () >= static_cast<int> (descriptors_.size ()))
    return {};

  const auto &descriptor = descriptors_.at (index.row ());
  if (role == Qt::DisplayRole || role == DescriptorNameRole)
    return descriptor->name_.to_qstring ();
  if (role == DescriptorRole)
    return QVariant::fromValue (descriptor.get ());
  return {};
}

QStringList
PluginCatalog::availableFormats () const
{
  QReadLocker lock (&lock_);
  return available_formats_;
}

PluginCatalog::ScanState
PluginCatalog::scan_state (
  Protocol::ProtocolType                                protocol,
  const utils::Utf8String                              &identifier,
  const std::function<bool (const PluginDescriptor &)> &needs_rescanning) const
{
  QReadLocker lock (&lock_);
  const auto  is_blacklisted = std::ranges::any_of (
    blacklisted_identifiers_,
    [protocol, &identifier] (const BlacklistEntry &entry) {
      return entry.protocol_ == protocol && entry.identifier_ == identifier;
    });
  if (is_blacklisted)
    return ScanState::Blacklisted;

  const auto matches_scan =
    [protocol,
     &identifier] (const std::unique_ptr<PluginDescriptor> &descriptor) {
      return descriptor->protocol_ == protocol
             && descriptor->source_identifier () == identifier;
    };
  if (std::ranges::find_if (descriptors_, matches_scan) == descriptors_.end ())
    return ScanState::NeedsScan;

  return std::ranges::any_of (
           descriptors_,
           [&] (const auto &descriptor) {
             return matches_scan (descriptor) && needs_rescanning (*descriptor);
           })
           ? ScanState::NeedsScan
           : ScanState::UpToDate;
}

std::vector<std::unique_ptr<PluginDescriptor>>
PluginCatalog::descriptors_for_identifier (
  Protocol::ProtocolType   protocol,
  const utils::Utf8String &identifier) const
{
  QReadLocker lock (&lock_);
  return descriptors_
         | std::views::filter (
           [protocol,
            &identifier] (const std::unique_ptr<PluginDescriptor> &descriptor) {
             return descriptor->protocol_ == protocol
                    && descriptor->source_identifier () == identifier;
           })
         | std::views::transform (
           [] (const std::unique_ptr<PluginDescriptor> &descriptor) {
             return utils::clone_unique (*descriptor);
           })
         | std::ranges::to<std::vector> ();
}

QString
PluginCatalog::descriptor_key (const PluginDescriptor &descriptor)
{
  return format_name (descriptor.protocol_) + QStringLiteral ("/")
         + descriptor.source_identifier ().to_qstring () + QStringLiteral ("/")
         + QString::number (descriptor.unique_id_) + QStringLiteral ("/")
         + QString::number (descriptor.juce_compat_deprecated_unique_id_);
}

bool
PluginCatalog::same_metadata (
  const PluginDescriptor &a,
  const PluginDescriptor &b)
{
  const auto content = [] (const PluginDescriptor &descriptor) {
    return std::tie (
      descriptor.author_, descriptor.name_, descriptor.descriptive_name_,
      descriptor.website_, descriptor.category_, descriptor.category_str_,
      descriptor.version_, descriptor.num_audio_ins_,
      descriptor.num_audio_outs_, descriptor.num_midi_ins_,
      descriptor.num_midi_outs_, descriptor.num_ctrl_ins_,
      descriptor.num_ctrl_outs_, descriptor.num_cv_ins_,
      descriptor.num_cv_outs_, descriptor.protocol_, descriptor.path_or_id_,
      descriptor.unique_id_, descriptor.juce_compat_deprecated_unique_id_,
      descriptor.arch_, descriptor.has_custom_ui_);
  };
  return content (a) == content (b);
}

void
PluginCatalog::refresh_scan_timestamps (
  PluginDescriptor       &stored,
  const PluginDescriptor &fresh)
{
  stored.last_file_mod_time_ms_ = fresh.last_file_mod_time_ms_;
  stored.last_info_update_time_ms_ = fresh.last_info_update_time_ms_;
}

bool
PluginCatalog::add_descriptor (std::unique_ptr<PluginDescriptor> descriptor)
{
  z_return_val_if_fail (descriptor != nullptr, false);

  const QString key = descriptor_key (*descriptor);

  int changed_row = -1;
  {
    QWriteLocker lock (&lock_);
    const auto   existing_it = std::ranges::find_if (
      descriptors_, [&] (const std::unique_ptr<PluginDescriptor> &existing) {
        return descriptor_key (*existing) == key;
      });

    if (existing_it != descriptors_.end ())
      {
        if (same_metadata (**existing_it, *descriptor))
          {
            refresh_scan_timestamps (**existing_it, *descriptor);
            return false;
          }
        changed_row = static_cast<int> (
          std::ranges::distance (descriptors_.begin (), existing_it));
        *existing_it = std::move (descriptor);
        update_available_formats ();
      }
    else
      {
        const int new_row = static_cast<int> (descriptors_.size ());
        // begin/end notifications may query the model (e.g. rowCount), so
        // the lock must not be held across them
        lock.unlock ();
        beginInsertRows ({}, new_row, new_row);
        lock.relock ();
        descriptors_.push_back (std::move (descriptor));
        lock.unlock ();
        endInsertRows ();
        lock.relock ();
        update_available_formats ();
        return true;
      }
  }

  Q_EMIT dataChanged (index (changed_row, 0), index (changed_row, 0));
  return false;
}

std::expected<void, utils::Utf8String>
PluginCatalog::replace_results_for_scan (
  Protocol::ProtocolType                         protocol,
  const utils::Utf8String                       &identifier,
  std::vector<std::unique_ptr<PluginDescriptor>> descriptors)
{
  z_return_val_if_fail (
    std::ranges::all_of (
      descriptors,
      [protocol,
       identifier] (const std::unique_ptr<PluginDescriptor> &descriptor) {
        return descriptor != nullptr && descriptor->protocol_ == protocol
               && descriptor->source_identifier () == identifier;
      }),
    std::unexpected (
      utils::Utf8String::from_utf8_encoded_string (
        "Scan results do not match the scanned protocol and identifier")));

  QHash<QString, int> fresh_by_key;
  for (const auto [row, descriptor] : descriptors | utils::views::enumerate)
    {
      const auto key = descriptor_key (*descriptor);
      if (fresh_by_key.contains (key))
        return std::unexpected (
          utils::Utf8String::from_utf8_encoded_string (
            fmt::format (
              "Scan results for '{}' contain duplicate plugin identities",
              identifier.view ())));
      fresh_by_key.insert (key, static_cast<int> (row));
    }

  std::vector<int> changed_rows;
  {
    QWriteLocker lock (&lock_);

    const auto belongs_to_scan =
      [protocol,
       identifier] (const std::unique_ptr<PluginDescriptor> &descriptor) {
        return descriptor->protocol_ == protocol
               && descriptor->source_identifier () == identifier;
      };
    const auto removed_rows =
      std::views::iota (0, static_cast<int> (descriptors_.size ()))
      | std::views::filter ([&] (int row) {
          const auto &descriptor = descriptors_[row];
          return belongs_to_scan (descriptor)
                 && !fresh_by_key.contains (descriptor_key (*descriptor));
        })
      | std::ranges::to<std::vector> ();
    for (
      const auto &run :
      removed_rows | std::views::chunk_by ([] (int a, int b) {
        return a + 1 == b;
      }) | std::views::reverse)
      {
        lock.unlock ();
        beginRemoveRows ({}, run.front (), run.back ());
        lock.relock ();
        descriptors_.erase (
          descriptors_.begin () + run.front (),
          descriptors_.begin () + run.back () + 1);
        lock.unlock ();
        endRemoveRows ();
        lock.relock ();
      }

    std::vector<bool> fresh_used (descriptors.size (), false);
    for (const auto [row, descriptor] : descriptors_ | utils::views::enumerate)
      {
        if (!belongs_to_scan (descriptor))
          continue;
        const auto fresh_row =
          fresh_by_key.value (descriptor_key (*descriptor), -1);
        if (fresh_row < 0)
          continue;
        fresh_used[static_cast<size_t> (fresh_row)] = true;
        auto &fresh_descriptor = descriptors[static_cast<size_t> (fresh_row)];
        if (same_metadata (*descriptor, *fresh_descriptor))
          {
            refresh_scan_timestamps (*descriptor, *fresh_descriptor);
            continue;
          }
        descriptors_[static_cast<size_t> (row)] = std::move (fresh_descriptor);
        changed_rows.push_back (static_cast<int> (row));
      }

    std::vector<std::unique_ptr<PluginDescriptor>> new_descriptors;
    for (const auto [row, used] : fresh_used | utils::views::enumerate)
      {
        if (!used)
          new_descriptors.push_back (
            std::move (descriptors[static_cast<size_t> (row)]));
      }
    if (!new_descriptors.empty ())
      {
        lock.unlock ();
        beginInsertRows (
          {}, static_cast<int> (descriptors_.size ()),
          static_cast<int> (descriptors_.size () + new_descriptors.size () - 1));
        lock.relock ();
        descriptors_.insert (
          descriptors_.end (),
          std::make_move_iterator (new_descriptors.begin ()),
          std::make_move_iterator (new_descriptors.end ()));
        lock.unlock ();
        endInsertRows ();
        lock.relock ();
      }
    update_available_formats ();
  }

  for (
    const auto &run :
    changed_rows | std::views::chunk_by ([] (int a, int b) {
      return a + 1 == b;
    }))
    {
      Q_EMIT dataChanged (index (run.front (), 0), index (run.back (), 0));
    }

  return {};
}

void
PluginCatalog::remove_descriptor (const PluginDescriptor &descriptor)
{
  const auto   key = descriptor_key (descriptor);
  QWriteLocker lock (&lock_);
  const auto   existing_it = std::ranges::find_if (
    descriptors_, [&] (const std::unique_ptr<PluginDescriptor> &existing) {
      return descriptor_key (*existing) == key;
    });
  if (existing_it == descriptors_.end ())
    return;

  const int row = static_cast<int> (
    std::ranges::distance (descriptors_.begin (), existing_it));
  lock.unlock ();
  beginRemoveRows ({}, row, row);
  lock.relock ();
  descriptors_.erase (existing_it);
  lock.unlock ();
  endRemoveRows ();
  lock.relock ();
  update_available_formats ();
}

void
PluginCatalog::clear ()
{
  beginResetModel ();
  {
    QWriteLocker lock (&lock_);
    descriptors_.clear ();
    blacklisted_identifiers_.clear ();
    update_available_formats ();
  }
  endResetModel ();
}

int
PluginCatalog::descriptor_count () const
{
  QReadLocker lock (&lock_);
  return static_cast<int> (descriptors_.size ());
}

std::vector<std::unique_ptr<PluginDescriptor>>
PluginCatalog::descriptors () const
{
  QReadLocker lock (&lock_);
  return descriptors_
         | std::views::transform (
           [] (const std::unique_ptr<PluginDescriptor> &descriptor) {
             return utils::clone_unique (*descriptor);
           })
         | std::ranges::to<std::vector> ();
}

bool
PluginCatalog::is_blacklisted (
  Protocol::ProtocolType   protocol,
  const utils::Utf8String &identifier) const
{
  QReadLocker lock (&lock_);
  return std::ranges::any_of (
    blacklisted_identifiers_,
    [protocol, &identifier] (const BlacklistEntry &entry) {
      return entry.protocol_ == protocol && entry.identifier_ == identifier;
    });
}

void
PluginCatalog::blacklist_identifier (
  Protocol::ProtocolType   protocol,
  const utils::Utf8String &identifier)
{
  QWriteLocker         lock (&lock_);
  const BlacklistEntry entry{ protocol, identifier };
  if (!std::ranges::contains (blacklisted_identifiers_, entry))
    blacklisted_identifiers_.push_back (entry);
}

void
PluginCatalog::remove_from_blacklist (
  Protocol::ProtocolType   protocol,
  const utils::Utf8String &identifier)
{
  QWriteLocker lock (&lock_);
  std::erase_if (blacklisted_identifiers_, [&] (const BlacklistEntry &entry) {
    return entry.protocol_ == protocol && entry.identifier_ == identifier;
  });
}

void
PluginCatalog::clear_blacklist ()
{
  QWriteLocker lock (&lock_);
  blacklisted_identifiers_.clear ();
}

nlohmann::json
PluginCatalog::serialize () const
{
  QReadLocker    lock (&lock_);
  nlohmann::json json{
    { utils::serialization::kDocumentTypeKey,  kDocumentType                 },
    { utils::serialization::kSchemaVersionKey,
     nlohmann::json{
        { "major", kMajorSchemaVersion }, { "minor", kMinorSchemaVersion } } },
    { kPluginsKey,                             descriptors_                  },
    { kBlacklistedIdentifiersKey,              nlohmann::json::array ()      },
  };
  for (const auto &entry : blacklisted_identifiers_)
    {
      json[kBlacklistedIdentifiersKey].push_back (
        nlohmann::json{
          { kProtocolKey,   static_cast<int> (entry.protocol_) },
          { kIdentifierKey, entry.identifier_                  }
      });
    }
  return json;
}

void
PluginCatalog::deserialize (const nlohmann::json &json)
{
  try
    {
      CatalogSchemaValidator::get_instance ().validate (json);
    }
  catch (const std::exception &e)
    {
      throw ZrythmException (
        fmt::format ("Known plugins JSON validation failed: {}", e.what ()));
    }

  const auto &format_version = json.at (utils::serialization::kSchemaVersionKey);
  if (format_version.at ("major").get<int> () > kMajorSchemaVersion)
    {
      throw ZrythmException (
        "Known plugins cache is from a newer format version");
    }

  std::vector<std::unique_ptr<PluginDescriptor>> descriptors;
  for (const auto &descriptor_json : json.at (kPluginsKey))
    {
      auto descriptor = std::make_unique<PluginDescriptor> ();
      from_json (descriptor_json, *descriptor);
      descriptors.push_back (std::move (descriptor));
    }

  std::vector<BlacklistEntry> blacklist;
  for (const auto &entry_json : json.at (kBlacklistedIdentifiersKey))
    {
      blacklist.push_back (
        BlacklistEntry{
          static_cast<Protocol::ProtocolType> (
            entry_json.at (kProtocolKey).get<int> ()),
          entry_json.at (kIdentifierKey).get<utils::Utf8String> (),
        });
    }

  beginResetModel ();
  {
    QWriteLocker lock (&lock_);
    descriptors_ = std::move (descriptors);
    blacklisted_identifiers_ = std::move (blacklist);
    update_available_formats ();
  }
  endResetModel ();
}

void
PluginCatalog::update_available_formats ()
{
  QStringList formats;
  for (const auto &descriptor : descriptors_)
    formats.push_back (format_name (descriptor->protocol_));
  formats.removeDuplicates ();
  formats.sort ();
  if (formats != available_formats_)
    {
      available_formats_ = formats;
      // The signal is delivered queued: callers hold the write lock, and a
      // directly connected listener that reads the model (e.g.
      // availableFormats()) would deadlock on the read lock
      QMetaObject::invokeMethod (
        this, [this] () { Q_EMIT availableFormatsChanged (); },
        Qt::QueuedConnection);
    }
}

} // namespace zrythm::plugins::discovery
