// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QObject>

// Mock QObject for parenting
class MockQObject : public QObject
{
  Q_OBJECT
public:
  MockQObject (QObject * parent = nullptr) : QObject (parent) { }
};
