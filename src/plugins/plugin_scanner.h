#pragma once

#include <QObject>

class PluginScanner : public QObject
{
  Q_OBJECT
  QML_ELEMENT

public:
  explicit PluginScanner (QObject * parent = nullptr);

public Q_SLOTS:
  void startScan ();

Q_SIGNALS:
  void progressTextChanged (const QString &text);
  void progressValueChanged (qreal value);

private:
  void performScan ();
};