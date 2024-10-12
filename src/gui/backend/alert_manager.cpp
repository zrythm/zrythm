#include "alert_manager.h"

using namespace zrythm::gui;

AlertManager::AlertManager (QObject * parent) : QObject (parent) { }

void
AlertManager::showAlert (const QString &title, const QString &message)
{
  Q_EMIT alertRequested (title, message);
}