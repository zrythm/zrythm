#pragma once

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace zrythm::gui
{

class AlertManager : public QObject
{
  Q_OBJECT
  QML_ELEMENT

public:
  explicit AlertManager (QObject * parent = nullptr);

  Q_INVOKABLE void showAlert (const QString &title, const QString &message);

Q_SIGNALS:
  void alertRequested (const QString &title, const QString &message);
};

} // namespace zrythm::gui