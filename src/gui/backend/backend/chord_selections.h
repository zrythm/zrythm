// SPDX-FileCopyrightText: © 2019, 2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_BACKEND_CHORD_SELECTIONS_H__
#define __GUI_BACKEND_CHORD_SELECTIONS_H__

#include "gui/backend/backend/arranger_selections.h"

/**
 * @addtogroup gui_backend
 *
 * @{
 */

#define CHORD_SELECTIONS (PROJECT->chord_selections_)

/**
 * Selections to be used for the ChordArrangerWidget's
 * current selections, copying, undoing, etc.
 */
class ChordSelections final
    : public QObject,
      public ArrangerSelections,
      public ICloneable<ChordSelections>,
      public ISerializable<ChordSelections>
{
  Q_OBJECT
  QML_ELEMENT
public:
  ChordSelections ();

  void sort_by_indices (bool desc) override;

  void init_after_cloning (const ChordSelections &other) override
  {
    ArrangerSelections::copy_members_from (other);
  }

  DECLARE_DEFINE_FIELDS_METHOD ();

private:
  bool can_be_pasted_at_impl (const Position pos, const int idx) const override;
};

/**
 * @}
 */

#endif
