// SPDX-FileCopyrightText: © 2021-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __UTILS_HASH_H__
#define __UTILS_HASH_H__

#include "utils/types.h"

#include <QFile>

#include <xxhash.h>

namespace zrythm::utils::hash
{
using HashT = XXH64_hash_t;

template <typename T>
HashT
get_custom_hash (const T * data, size_t size)
{
  return XXH3_64bits (data, size);
}

// Hash any object
template <typename T>
HashT
get_object_hash (const T &obj)
{
  return get_custom_hash (&obj, sizeof (T));
}

// Hash string contents
inline HashT
get_string_hash (const std::string &str)
{
  return get_custom_hash (str.data (), str.size ());
}

// Hash a file
HashT
get_file_hash (const std::filesystem::path &path);

// Get canonical string representation of a hash
std::string
to_string (HashT hash);

}; // namespace zrythm::utils::hash

#endif
