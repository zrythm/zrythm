// SPDX-FileCopyrightText: © 2020-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/arranger_object_span.h"
#include "structure/tracks/track_span.h"

/**
 * @addtogroup gui_backend
 */

/**
 * Clipboard struct.
 */
class Clipboard final
{
public:
  using PluginUuid = plugins::Plugin::Uuid;

  /**
   * Clipboard type.
   */
  enum class Type
  {
    ArrangerObjects,
    Plugins,
    Tracks,
  };

public:
  Clipboard () = default;
  Clipboard (std::ranges::range auto arranger_objects)
    requires std::is_same_v<
               decltype (*arranger_objects.begin ()),
               structure::arrangement::ArrangerObjectPtrVariant>
      : type_ (Type::ArrangerObjects),
        arranger_objects_ (std::ranges::to (arranger_objects))
  {
  }

  Clipboard (std::ranges::range auto plugins)
    requires std::is_same_v<decltype (*plugins.begin ()), plugins::PluginPtrVariant>

      : type_ (Type::Plugins), plugins_ (std::ranges::to (plugins))
  {
  }

  /**
   * @brief Construct a new Clipboard object
   *
   * @param sel
   * @throw ZrythmException on error.
   */
  Clipboard (std::ranges::range auto tracks)
    requires std::is_same_v<decltype (*tracks.begin ()), structure::tracks::TrackPtrVariant>
      : type_ (Type::Tracks), tracks_ (std::ranges::to (tracks))
  {
  }

  std::string get_document_type () const { return "ZrythmClipboard"; }
  int         get_format_major_version () const { return 3; }
  int         get_format_minor_version () const { return 0; }

private:
  NLOHMANN_DEFINE_TYPE_INTRUSIVE (
    Clipboard,
    type_,
    arranger_objects_,
    tracks_,
    plugins_)

public:
  Type                                                      type_{};
  std::vector<structure::arrangement::ArrangerObject::Uuid> arranger_objects_;
  std::vector<structure::tracks::Track::Uuid>               tracks_;
  std::vector<PluginUuid>                                   plugins_;
};

/**
 * @}
 */
