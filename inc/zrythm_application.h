// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
/* SPDX-License-Identifier: LicenseRef-ZrythmLicense */

#ifndef ZRYTHM_APPLICATION_H_INCLUDED
#define ZRYTHM_APPLICATION_H_INCLUDED

#include "zrythm-config.h"

#include <QApplication>

class ZrythmApplication : public QApplication
{
  Q_OBJECT

public:
  ZrythmApplication (int &argc, char ** argv);
};

/**
 * @}
 */

#endif /* ZRYTHM_APPLICATION_H_INCLUDED */
