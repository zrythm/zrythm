// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QObject>

class ConvertToVariantQObjBase : public QObject
{
  Q_OBJECT
public:
  explicit ConvertToVariantQObjBase (QObject * parent = nullptr)
      : QObject (parent)
  {
  }
};

class ConvertToVariantQObjBaseA : public ConvertToVariantQObjBase
{
  Q_OBJECT
public:
  explicit ConvertToVariantQObjBaseA (QObject * parent = nullptr)
      : ConvertToVariantQObjBase (parent)
  {
  }
};

class ConvertToVariantQObjBaseB : public ConvertToVariantQObjBase
{
  Q_OBJECT
public:
  explicit ConvertToVariantQObjBaseB (QObject * parent = nullptr)
      : ConvertToVariantQObjBase (parent)
  {
  }
};

class ConvertToVariantQObjOther : public ConvertToVariantQObjBase
{
  Q_OBJECT
public:
  explicit ConvertToVariantQObjOther (QObject * parent = nullptr)
      : ConvertToVariantQObjBase (parent)
  {
  }
};
