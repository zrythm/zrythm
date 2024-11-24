// SPDX-FileCopyrightText: © 2021-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <memory>
#include <span>

#include "utils/hash.h"
#include "utils/io.h"
#include "utils/logger.h"

namespace zrythm::utils::hash
{

namespace
{
class StreamingHash
{
public:
  StreamingHash () : state_ (XXH3_createState (), XXH3_freeState)
  {
    XXH3_64bits_reset (state_.get ());
  }

  void update (std::span<const std::byte> data)
  {
    XXH3_64bits_update (state_.get (), data.data (), data.size ());
  }

  HashT finalize () const { return XXH3_64bits_digest (state_.get ()); }

private:
  std::unique_ptr<XXH3_state_t, decltype (&XXH3_freeState)> state_;
};
}

HashT
get_file_hash (const std::filesystem::path &path)
{
  StreamingHash hasher;

  QFile file (QString::fromStdString (path.string ()));
  if (!file.open (QIODevice::ReadOnly))
    {
      z_warning ("Failed to open file: {}", path.string ());
      return 0;
    }

  constexpr size_t buf_size = 16384;
  QByteArray       buf;
  while (!(buf = file.read (buf_size)).isEmpty ())
    {
      hasher.update (std::span<const std::byte>{
        reinterpret_cast<const std::byte *> (buf.data ()),
        static_cast<size_t> (buf.size ()) });
    }

  return hasher.finalize ();
}

std::string
to_string (HashT hash)
{
  XXH64_canonical_t canonical;
  XXH64_canonicalFromHash (&canonical, hash);

  return fmt::format (
    "{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}", canonical.digest[0],
    canonical.digest[1], canonical.digest[2], canonical.digest[3],
    canonical.digest[4], canonical.digest[5], canonical.digest[6],
    canonical.digest[7]);
}
}
