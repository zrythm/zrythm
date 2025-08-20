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

#include "plugins/clap_plugin_param.h"

namespace zrythm::plugins
{
ClapPluginParam::
  ClapPluginParam (const clap_param_info &info, double value, QObject * parent)
    : QObject (parent), _info (info), _value (value)
{
}

void
ClapPluginParam::setValue (double v)
{
  if (_value == v)
    return;

  _value = v;
  valueChanged ();
}

void
ClapPluginParam::setModulation (double v)
{
  if (_modulation == v)
    return;

  _modulation = v;
  modulatedValueChanged ();
}

bool
ClapPluginParam::isValueValid (const double v) const
{
  return _info.min_value <= v && v <= _info.max_value;
}

void
ClapPluginParam::printShortInfo (std::ostream &os) const
{
  os << "id: " << _info.id << ", name: '" << _info.name << "', module: '"
     << _info.module << "'";
}

void
ClapPluginParam::printInfo (std::ostream &os) const
{
  printShortInfo (os);
  os << ", min: " << _info.min_value << ", max: " << _info.max_value;
}

bool
ClapPluginParam::isInfoEqualTo (const clap_param_info &info) const
{
  return info.cookie == _info.cookie && info.default_value == _info.default_value
         && info.max_value == _info.max_value && info.min_value == _info.min_value
         && info.flags == _info.flags && info.id == _info.id
         && (::std::strncmp (info.name, _info.name, sizeof (info.name)) == 0)
         && (::std::strncmp (info.module, _info.module, sizeof (info.module)) == 0);
}

bool
ClapPluginParam::isInfoCriticallyDifferentTo (const clap_param_info &info) const
{
  assert (_info.id == info.id);
  const uint32_t criticalFlags =
    CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_AUTOMATABLE_PER_NOTE_ID
    | CLAP_PARAM_IS_AUTOMATABLE_PER_KEY | CLAP_PARAM_IS_AUTOMATABLE_PER_CHANNEL
    | CLAP_PARAM_IS_AUTOMATABLE_PER_PORT | CLAP_PARAM_IS_MODULATABLE
    | CLAP_PARAM_IS_MODULATABLE_PER_NOTE_ID | CLAP_PARAM_IS_MODULATABLE_PER_KEY
    | CLAP_PARAM_IS_MODULATABLE_PER_CHANNEL | CLAP_PARAM_IS_MODULATABLE_PER_PORT
    | CLAP_PARAM_IS_READONLY | CLAP_PARAM_REQUIRES_PROCESS;
  return (_info.flags & criticalFlags) == (info.flags & criticalFlags)
         || _info.min_value != _info.min_value
         || _info.max_value != _info.max_value;
}
}
