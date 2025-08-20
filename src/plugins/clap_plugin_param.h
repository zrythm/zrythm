// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * ---
 *
 * SPDX-FileCopyrightText: Copyright (c) 2021 Alexandre BIQUE
 * SPDX-License-Identifier: MIT
 *
 * Copyright (c) 2021 Alexandre BIQUE
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * ---
 *
 */

#pragma once

#include <ostream>
#include <unordered_map>

#include <QObject>

#include <clap/clap.h>

namespace zrythm::plugins
{
class ClapHost;

class ClapPluginParam : public QObject
{
  Q_OBJECT

public:
  ClapPluginParam (
    const clap_param_info &info,
    double                 value,
    QObject *              parent = nullptr);

  double value () const { return _value; }
  void   setValue (double v);

  double modulation () const { return _modulation; }
  void   setModulation (double v);

  double modulatedValue () const
  {
    return std::min (
      _info.max_value, std::max (_info.min_value, _value + _modulation));
  }

  bool isValueValid (const double v) const;

  void printShortInfo (std::ostream &os) const;
  void printInfo (std::ostream &os) const;

  void setInfo (const clap_param_info &info) noexcept { _info = info; }
  bool isInfoEqualTo (const clap_param_info &info) const;
  bool isInfoCriticallyDifferentTo (const clap_param_info &info) const;
  clap_param_info       &info () noexcept { return _info; }
  const clap_param_info &info () const noexcept { return _info; }

  bool isBeingAdjusted () const noexcept { return _isBeingAdjusted; }
  void setIsAdjusting (bool isAdjusting)
  {
    if (isAdjusting && !_isBeingAdjusted)
      beginAdjust ();
    else if (!isAdjusting && _isBeingAdjusted)
      endAdjust ();
  }
  void beginAdjust ()
  {
    Q_ASSERT (!_isBeingAdjusted);
    _isBeingAdjusted = true;
    Q_EMIT isBeingAdjustedChanged ();
  }
  void endAdjust ()
  {
    Q_ASSERT (_isBeingAdjusted);
    _isBeingAdjusted = false;
    Q_EMIT isBeingAdjustedChanged ();
  }

Q_SIGNALS:
  void isBeingAdjustedChanged ();
  void infoChanged ();
  void valueChanged ();
  void modulatedValueChanged ();

private:
  bool                                     _isBeingAdjusted = false;
  clap_param_info                          _info;
  double                                   _value = 0;
  double                                   _modulation = 0;
  std::unordered_map<int64_t, std::string> _enumEntries;
};
}
