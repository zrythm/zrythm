// SPDX-FileCopyrightText: © 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * File import handling.
 */

#ifndef __IO_FILE_IMPORT_H__
#define __IO_FILE_IMPORT_H__

#include <utility>

#include "dsp/position.h"
#include "utils/types.h"

#include <glib.h>

G_BEGIN_DECLS

/**
 * @addtogroup io
 *
 * @{
 */

#define FILE_IMPORT_TYPE (file_import_get_type ())
G_DECLARE_FINAL_TYPE (FileImport, file_import, Z, FILE_IMPORT, GObject);

struct FileImportInfo
{
public:
  FileImportInfo () = default;
  FileImportInfo (
    unsigned int track_name_hash,
    int          lane,
    Position     pos,
    int          track_idx)
      : track_name_hash_ (track_name_hash), lane_ (lane),
        pos_ (std::move (pos)), track_idx_ (track_idx)
  {
  }

  /** Track to import on, if any, or 0. */
  unsigned int track_name_hash_;

  /** Track lane to import on, if any, or -1. */
  int lane_;

  /** Position to import the data at, or 1.1.1.0. */
  Position pos_;

  /** Track index to start the import at. */
  int track_idx_;
};

/**
 * An object used for importing files asynchronously.
 */
using FileImport = struct _FileImport
{
  GObject parent_instance;

  /** File path. */
  std::string filepath;

  /** Return value. */
  std::vector<std::shared_ptr<Region>> regions;

  /**
   * Owner of this FileImport instance, set to the GTask.
   *
   * This is to ensure that the internal GTask instance doesn't
   * attempt to call the async ready callback if the owner
   * is already destroyed.
   */
  GObject * owner;

  /** Import info. */
  std::unique_ptr<FileImportInfo> import_info;
};

G_END_DECLS

/**
 * Returns a new FileImport instance.
 */
FileImport *
file_import_new (const std::string &filepath, const FileImportInfo * import_nfo);

/**
 * Begins file import for a single file.
 *
 * @param owner Passed to the task as the owner object. This is
 *   to avoid the task callback being called after the owner
 *   object is deleted.
 * @param callback_data User data to pass to the callback.
 */
void
file_import_async (
  FileImport *        self,
  GObject *           owner,
  GCancellable *      cancellable,
  GAsyncReadyCallback callback,
  gpointer            callback_data);

std::vector<std::shared_ptr<Region>>
file_import_sync (FileImport * self, GError ** error);

/**
 * To be called by the provided GAsyncReadyCallback to retrieve
 * retun values and error details, passing the GAsyncResult
 * which was passed to the callback.
 *
 * @return A pointer array of regions
 */
std::vector<std::shared_ptr<Region>>
file_import_finish (FileImport * self, GAsyncResult * result, GError ** error);

/**
 * @}
 */

#endif
