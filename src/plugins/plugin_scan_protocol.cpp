// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "plugins/plugin_scan_protocol.h"
#include "utils/exceptions.h"
#include "utils/serialization.h"

#include <fmt/format.h>
#include <nlohmann/json.hpp>

namespace zrythm::plugins::discovery
{

namespace
{
constexpr auto kDocumentType = "ZrythmPluginScanResponse"sv;
constexpr auto kMajorSchemaVersion = 1;
constexpr auto kMinorSchemaVersion = 0;
constexpr auto kSuccessKey = "success"sv;
constexpr auto kErrorMessageKey = "errorMessage"sv;
constexpr auto kPluginsKey = "plugins"sv;

nlohmann::json
schema_version_json ()
{
  return nlohmann::json{
    { "major", kMajorSchemaVersion },
    { "minor", kMinorSchemaVersion }
  };
}
} // namespace

utils::Utf8String
serialize_plugin_scan_response (const PluginScanResult &result)
{
  nlohmann::json json{
    { utils::serialization::kDocumentTypeKey,  kDocumentType            },
    { utils::serialization::kSchemaVersionKey, schema_version_json ()   },
    { kSuccessKey,                             result.has_value ()      },
    { kErrorMessageKey,
     result.has_value () ? utils::Utf8String{} : result.error ()        },
    { kPluginsKey,                             nlohmann::json::array () },
  };

  if (result.has_value ())
    {
      for (const auto &descriptor : result.value ())
        json[kPluginsKey].push_back (*descriptor);
    }

  return utils::Utf8String::from_utf8_encoded_string (json.dump ());
}

PluginScanResult
deserialize_plugin_scan_response (std::string_view payload)
{
  try
    {
      const auto json = nlohmann::json::parse (payload);
      if (
        json.at (utils::serialization::kDocumentTypeKey).get<std::string> ()
        != kDocumentType)
        return std::unexpected (
          utils::Utf8String::from_utf8_encoded_string (
            "Unexpected plugin scan response document type"));

      const auto &version = json.at (utils::serialization::kSchemaVersionKey);
      if (version.at ("major").get<int> () > kMajorSchemaVersion)
        return std::unexpected (
          utils::Utf8String::from_utf8_encoded_string (
            "Plugin scan response is from a newer format version"));

      if (!json.at (kSuccessKey).get<bool> ())
        return std::unexpected (
          json.at (kErrorMessageKey).get<utils::Utf8String> ());

      std::vector<std::unique_ptr<PluginDescriptor>> descriptors;
      for (const auto &descriptor_json : json.at (kPluginsKey))
        {
          auto descriptor = std::make_unique<PluginDescriptor> ();
          from_json (descriptor_json, *descriptor);
          descriptors.push_back (std::move (descriptor));
        }
      return descriptors;
    }
  catch (const std::exception &e)
    {
      return std::unexpected (
        utils::Utf8String::from_utf8_encoded_string (
          fmt::format ("Invalid plugin scan response: {}", e.what ())));
    }
}

} // namespace zrythm::plugins::discovery
