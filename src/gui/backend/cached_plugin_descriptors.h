// SPDX-FileCopyrightText: © 2020-2021, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once
#include "gui/dsp/plugin_descriptor.h"
#include "utils/serialization.h"

/**
 * @addtogroup plugins
 *
 * @{
 */

namespace zrythm::gui::old_dsp::plugins
{

/**
 * Descriptors to be cached.
 */
class CachedPluginDescriptors final
{
public:
  /**
   * @brief Returns a new instance with the cache pre-filled from the cache
   * file, or an empty instance if there is no file or an error occurred.
   *
   * @return std::unique_ptr<CachedPluginDescriptors>
   */
  static std::unique_ptr<CachedPluginDescriptors> read_or_new ();

  /**
   * @brief Serializes the cached descriptors to the standard file.
   *
   * @throw ZrythmException if an error occurred while serializing.
   */
  void serialize_to_file ();

  /**
   * @brief Wrapper over @ref serialize_to_file that ignores exceptions.
   *
   */
  void serialize_to_file_no_throw ()
  {
    try
      {
        serialize_to_file ();
      }
    catch (const ZrythmException &e)
      {
        z_warning (e.what ());
      }
  }

  static constexpr int  get_format_major_version () { return 6; }
  static constexpr int  get_format_minor_version () { return 0; }
  static constexpr auto get_document_type ()
  {
    return "CachedPluginDescriptors"sv;
  }

  /**
   * Returns if the plugin with the given sha1 is blacklisted or not.
   */
  bool is_blacklisted (std::string_view sha1);

  /**
   * Returns found non-blacklisted descriptors.
   */
  std::vector<std::unique_ptr<PluginDescriptor>>
  get_valid_descriptors_for_sha1 (std::string_view sha1);

  /**
   * @brief Returns whether the given sha1 is in the cache.
   *
   * @param sha1
   */
  bool
  contains_sha1 (std::string_view sha1, bool check_valid, bool check_blacklisted);

  bool contains_valid_sha1 (auto sha1)
  {
    return contains_sha1 (sha1, true, false);
  }

  bool contains_blacklisted_sha1 (auto sha1)
  {
    return contains_sha1 (sha1, false, true);
  }

  /**
   * Appends a descriptor to the cache.
   *
   * @param serialize Whether serialize the updated cache now.
   */
  void blacklist (std::string_view sha1, bool serialize);

  /**
   * Appends a descriptor to the cache.
   *
   * @param serialize Whether to serialize the updated
   *   cache now.
   */
  void add (const PluginDescriptor &descr, bool serialize);

  /**
   * Clears the descriptors and removes the cache file.
   */
  void clear ();

  static constexpr auto kDescriptorsKey = "descriptors"sv;
  static constexpr auto kBlacklistedSha1sKey = "blacklistedSha1s"sv;

private:
  friend void to_json (nlohmann::json &j, const CachedPluginDescriptors &p)
  {
    j = nlohmann::json{
      { utils::serialization::kDocumentTypeKey, get_document_type ()        },
      { utils::serialization::kFormatMajorKey,  get_format_major_version () },
      { utils::serialization::kFormatMinorKey,  get_format_minor_version () },
      { kDescriptorsKey,                        p.descriptors_              },
      { kBlacklistedSha1sKey,                   p.blacklisted_sha1s_        },
    };
  }
  friend void from_json (const nlohmann::json &j, CachedPluginDescriptors &p);

  /**
   * @brief Deletes the cache file.
   *
   * @throw ZrythmException if an error occurred while deleting the file.
   */
  static void delete_file ();

  static fs::path get_file_path ();

public:
  /** Valid descriptors. */
  std::vector<std::unique_ptr<PluginDescriptor>> descriptors_;

  /** Blacklisted hashes, to skip when scanning. */
  std::vector<std::string> blacklisted_sha1s_;
};

} // namespace zrythm::gui::old_dsp::plugins

/**
 * @}
 */
