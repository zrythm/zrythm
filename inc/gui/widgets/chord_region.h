/*
 * SPDX-FileCopyrightText: © 2019 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

/**
 * \file
 *
 * Widget for MIDI regions, inheriting from
 * RegionWidget.
 */

#ifndef __GUI_WIDGETS_CHORD_REGION_H__
#define __GUI_WIDGETS_CHORD_REGION_H__

#include "dsp/region.h"
#include "gui/widgets/region.h"
#include "utils/ui.h"

#include "gtk_wrapper.h"

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * Recreates the pango layout for drawing chord
 * names inside the region.
 */
void
chord_region_recreate_pango_layouts (Region * self);

#if 0

/**
 * A widget that represents a Region in the
 * TimelineArrangerWidget.
 *
 * It displays the MidiNotes of the Region in
 * miniature size.
 */
typedef struct _ChordRegionWidget
{
  RegionWidget             parent_instance;
} ChordRegionWidget;

/**
 * Creates a region.
 */
ChordRegionWidget *
chord_region_widget_new (
  Region * chord_region);

#endif

/**
 * @}
 */

#endif
