// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __DSP_PROJECT_OWNED_OBJECT_H__
#define __DSP_PROJECT_OWNED_OBJECT_H__

class IProjectOwnedObject
{
public:
  virtual ~IProjectOwnedObject () = default;

  virtual bool is_in_active_project () const = 0;
  virtual bool is_auditioner () const = 0;
};

#endif // __DSP_PROJECT_OWNED_OBJECT_H__