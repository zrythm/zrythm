// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: © 2024 Miró Allard <miro.allard@pm.me>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#if 0
void
ui_set_pointer_cursor (GtkWidget * widget)
{
  ui_set_cursor_from_icon_name (GTK_WIDGET (widget), "select-cursor", 3, 1);
}
#endif

std::string
ui_get_db_value_as_string (float val)
{
  if (val < -98.f)
    {
      return { "-∞" };
    }

  if (val < -10.)
    {
      return fmt::format ("{:.0f}", val);
    }

  return fmt::format ("{:.1f}", val);
}
