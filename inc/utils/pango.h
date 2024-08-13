/*
 * SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

/**
 * @file
 *
 * Pango utils.
 */

#ifndef __UTILS_PANGO_H__
#define __UTILS_PANGO_H__

#include <memory>

#include "gtk_wrapper.h"

/**
 * @addtogroup utils
 *
 * @{
 */

struct PangoLayoutDeleter
{
  PangoLayoutDeleter () {};

  void operator() (PangoLayout * layout) const { g_object_unref (layout); };
};

using PangoLayoutUniquePtr = std::unique_ptr<PangoLayout, PangoLayoutDeleter>;

PangoLayoutUniquePtr
z_pango_create_layout_from_description (
  GtkWidget *            widget,
  PangoFontDescription * descr);

/**
 * @}
 */

#endif /* __UTILS_PANGO_H__ */
