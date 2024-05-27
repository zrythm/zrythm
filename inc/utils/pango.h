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

#include "gtk_wrapper.h"

/**
 * @addtogroup utils
 *
 * @{
 */

PangoLayout *
z_pango_create_layout_from_description (
  GtkWidget *            widget,
  PangoFontDescription * descr);

/**
 * @}
 */

#endif /* __UTILS_PANGO_H__ */
