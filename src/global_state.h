#pragma once

#include "gui/backend/backend/settings_manager.h"
#include "gui/backend/backend/theme_manager.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/backend/project_manager.h"

class GlobalState : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON
public:
  Q_PROPERTY (Zrythm * zrythm READ getZrythm CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::gui::ThemeManager * themeManager READ getThemeManager CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::gui::SettingsManager * settingsManager READ getSettingsManager
      CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::gui::ProjectManager * projectManager READ getProjectManager CONSTANT
      FINAL)
public:
  GlobalState (QObject * parent = nullptr) : QObject (parent) { }
  Zrythm *                       getZrythm ();
  zrythm::gui::ThemeManager *    getThemeManager ();
  zrythm::gui::SettingsManager * getSettingsManager ();
  zrythm::gui::ProjectManager *  getProjectManager ();
};