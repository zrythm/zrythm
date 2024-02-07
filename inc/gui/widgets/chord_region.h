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

#include <gtk/gtk.h>

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
chord_region_recreate_pango_layouts (ZRegion * self);

#if 0

/**
 * A widget that represents a ZRegion in the
 * TimelineArrangerWidget.
 *
 * It displays the MidiNotes of the ZRegion in
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
  ZRegion * chord_region);

#endif

/**
 * @}
 */

#endif
