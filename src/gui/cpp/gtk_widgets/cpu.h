// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Cpu widget.
 */

#ifndef __GUI_WIDGETS_CPU_H__
#define __GUI_WIDGETS_CPU_H__

#include "gui/cpp/gtk_widgets/gtk_wrapper.h"

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_CPU (MW_BOT_BAR->cpu_load)

#define CPU_WIDGET_TYPE (cpu_widget_get_type ())
G_DECLARE_FINAL_TYPE (CpuWidget, cpu_widget, Z, CPU_WIDGET, GtkWidget)

typedef struct _CpuWidget
{
  GtkWidget parent_instance;

  /** CPU load (0-100). */
  int cpu;

  /** DSP load (0-100). */
  int dsp;

  /** Source func IDs. */
  guint cpu_source_id;
  guint dsp_source_id;

  GdkTexture * cpu_texture;
  GdkTexture * dsp_texture;
} CpuWidget;

/**
 * @}
 */

#endif
