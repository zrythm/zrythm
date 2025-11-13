// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QObject>

// Mock QObject for testing
class MockQObject : public QObject
{
  Q_OBJECT
  Q_PROPERTY (
    int intValue READ intValue WRITE setIntValue NOTIFY intValueChanged)
  Q_PROPERTY (
    QString stringValue READ stringValue WRITE setStringValue NOTIFY
      stringValueChanged)
  Q_PROPERTY (
    double doubleValue READ doubleValue WRITE setDoubleValue NOTIFY
      doubleValueChanged)

public:
  MockQObject (QObject * parent = nullptr) : QObject (parent) { }

  int  intValue () const { return int_value_; }
  void setIntValue (int value)
  {
    if (int_value_ != value)
      {
        int_value_ = value;
        Q_EMIT intValueChanged (value);
      }
  }

  QString stringValue () const { return string_value_; }
  void    setStringValue (const QString &value)
  {
    if (string_value_ != value)
      {
        string_value_ = value;
        Q_EMIT stringValueChanged (value);
      }
  }

  double doubleValue () const { return double_value_; }
  void   setDoubleValue (double value)
  {
    if (double_value_ != value)
      {
        double_value_ = value;
        Q_EMIT doubleValueChanged (value);
      }
  }

Q_SIGNALS:
  void intValueChanged (int value);
  void stringValueChanged (const QString &value);
  void doubleValueChanged (double value);

private:
  int     int_value_ = 42;
  QString string_value_ = QStringLiteral ("initial");
  double  double_value_ = 3.14;
};
