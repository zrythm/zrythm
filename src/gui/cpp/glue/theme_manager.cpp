// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/utils/directory_manager.h"
#include "gui/cpp/glue/theme_manager.h"
#include "gui/cpp/zrythm_application.h"

using namespace zrythm::gui::glue;

ThemeManager::ThemeManager (QObject * parent) : QObject (parent)
{
  // default palette
  palette_.setColor (QPalette::Window, QColor (32, 32, 32));
  palette_.setColor (QPalette::WindowText, Qt::white);
  palette_.setColor (QPalette::Base, QColor (55, 55, 55));
  palette_.setColor (QPalette::AlternateBase, QColor (45, 45, 45));
  palette_.setColor (QPalette::ToolTipBase, Qt::white);
  palette_.setColor (QPalette::ToolTipText, Qt::white);
  palette_.setColor (QPalette::Text, Qt::white);
  palette_.setColor (QPalette::Button, QColor (55, 55, 55));
  palette_.setColor (QPalette::ButtonText, Qt::white);
  palette_.setColor (QPalette::BrightText, Qt::red);
  palette_.setColor (QPalette::Link, QColor (42, 130, 218));
  palette_.setColor (QPalette::Highlight, QColor (42, 130, 218));
  palette_.setColor (QPalette::HighlightedText, Qt::black);

  auto themes_dir = DirectoryManager::getInstance ()->get_dir (
    DirectoryManager::DirectoryType::USER_THEMES);
  if (!fs::is_directory (themes_dir))
    {
      return;
    }
}

ThemeManager *
ThemeManager::get_instance ()
{
  return dynamic_cast<ZrythmApplication *> (qApp)->get_theme_manager ();
}