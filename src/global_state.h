#pragma once

#include "gui/backend/backend/zrythm.h"

class GlobalState : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON
public:
  Q_PROPERTY (Zrythm * zrythm READ getZrythm CONSTANT FINAL)
public:
  GlobalState (QObject * parent = nullptr) : QObject (parent) { }
  Zrythm * getZrythm ();
};