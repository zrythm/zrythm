// Copyright 2016 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include <lv2/atom/atom.h>
#include <lv2/atom/forge.h>
#include <lv2/atom/util.h>

#include <stdint.h>
#include <string.h>

/**
   A forge sink that writes to an atom buffer.

   It is assumed that the handle points to an LV2_Atom large enough to store
   the forge output.  The forged result is in the body of the buffer atom.
*/
static LV2_Atom_Forge_Ref
atom_sink(LV2_Atom_Forge_Sink_Handle handle, const void* buf, uint32_t size)
{
  LV2_Atom*      atom   = (LV2_Atom*)handle;
  const uint32_t offset = lv2_atom_total_size(atom);
  memcpy((char*)atom + offset, buf, size);
  atom->size += size;
  return offset;
}

/**
   Dereference counterpart to atom_sink().
*/
static LV2_Atom*
atom_sink_deref(LV2_Atom_Forge_Sink_Handle handle, LV2_Atom_Forge_Ref ref)
{
  return (LV2_Atom*)((char*)handle + ref);
}
