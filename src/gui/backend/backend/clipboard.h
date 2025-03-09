// SPDX-FileCopyrightText: © 2020-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_BACKEND_CLIPBOARD_H__
#define __GUI_BACKEND_CLIPBOARD_H__

#include "gui/backend/backend/arranger_selections.h"
#include "gui/dsp/track_span.h"

/**
 * @addtogroup gui_backend
 */

/**
 * Clipboard struct.
 */
class Clipboard
  final : public zrythm::utils::serialization::ISerializable<Clipboard>
{
public:
  using PluginUuid = Plugin::PluginUuid;

  /**
   * Clipboard type.
   */
  enum class Type
  {
    TimelineSelections,
    MidiSelections,
    AutomationSelections,
    ChordSelections,
    AudioSelections,
    PluginSelections,
    TrackCollection,
  };

public:
  Clipboard () = default;
  Clipboard (const ArrangerSelections &sel);

  Clipboard (std::ranges::range auto plugins)
    requires std::is_same_v<decltype (*plugins.begin ()), PluginPtrVariant>

      : type_ (Type::PluginSelections), plugins_ (std::ranges::to (plugins))
  {
  }

  /**
   * @brief Construct a new Clipboard object
   *
   * @param sel
   * @throw ZrythmException on error.
   */
  Clipboard (std::ranges::range auto tracks)
    requires std::is_same_v<decltype (*tracks.begin ()), TrackPtrVariant>
      : type_ (Type::TrackCollection), tracks_ (std::ranges::to (tracks))
  {
  }

  /**
   * Gets the ArrangerSelections, if this clipboard contains arranger
   * selections.
   */
  ArrangerSelections * get_selections () const;

  DECLARE_DEFINE_FIELDS_METHOD ();

  std::string get_document_type () const override { return "ZrythmClipboard"; };
  int         get_format_major_version () const override { return 3; }
  int         get_format_minor_version () const override { return 0; }

private:
  void set_type_from_arranger_selections (const ArrangerSelections &sel);

public:
  Type                                 type_{};
  std::unique_ptr<ArrangerSelections>  arranger_sel_;
  std::vector<Track::TrackUuid>        tracks_;
  std::vector<PluginUuid>              plugins_;
};

/**
 * @}
 */

#endif
