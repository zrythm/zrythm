// SPDX-FileCopyrightText: © 2020-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_BACKEND_CLIPBOARD_H__
#define __GUI_BACKEND_CLIPBOARD_H__

#include "gui/backend/backend/arranger_selections.h"
#include "gui/backend/backend/mixer_selections.h"
#include "gui/backend/backend/tracklist_selections.h"
#include "gui/backend/gtk_widgets/gtk_wrapper.h"

/**
 * @addtogroup gui_backend
 */

/**
 * Clipboard struct.
 */
class Clipboard final : public ISerializable<Clipboard>
{
public:
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
    MixerSelections,
    TracklistSelections,
  };

public:
  Clipboard () = default;
  Clipboard (const ArrangerSelections &sel);
  Clipboard (const MixerSelections &sel);

  /**
   * @brief Construct a new Clipboard object
   *
   * @param sel
   * @throw ZrythmException on error.
   */
  Clipboard (const SimpleTracklistSelections &sel);

  /**
   * Gets the ArrangerSelections, if this clipboard contains arranger
   * selections.
   */
  ArrangerSelections * get_selections () const;

  DECLARE_DEFINE_FIELDS_METHOD ();

  std::string get_document_type () const override { return "ZrythmClipboard"; };
  int         get_format_major_version () const override { return 2; }
  int         get_format_minor_version () const override { return 0; }

private:
  void set_type_from_arranger_selections (const ArrangerSelections &sel);

public:
  Type                                 type_;
  std::unique_ptr<ArrangerSelections>  arranger_sel_;
  std::unique_ptr<FullMixerSelections> mixer_sel_;
  std::unique_ptr<TracklistSelections> tracklist_sel_;
};

/**
 * @}
 */

#endif
