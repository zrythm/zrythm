#include "gui/backend/zrythm_application.h"

#include "global_state.h"

Zrythm *
GlobalState::getZrythm ()
{
  return Zrythm::getInstance ();
}

zrythm::gui::ThemeManager *
GlobalState::getThemeManager ()
{
  return dynamic_cast<zrythm::gui::ZrythmApplication *> (qApp)
    ->get_theme_manager ();
}

zrythm::gui::SettingsManager *
GlobalState::getSettingsManager ()
{
  return dynamic_cast<zrythm::gui::ZrythmApplication *> (qApp)
    ->get_settings_manager ();
}

zrythm::gui::ProjectManager *
GlobalState::getProjectManager ()
{
  return dynamic_cast<zrythm::gui::ZrythmApplication *> (qApp)
    ->get_project_manager ();
}

zrythm::gui::AlertManager *
GlobalState::getAlertManager ()
{
  return dynamic_cast<zrythm::gui::ZrythmApplication *> (qApp)
    ->get_alert_manager ();
}