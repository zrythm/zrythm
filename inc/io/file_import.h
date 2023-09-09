// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * File import handling.
 */

#ifndef __IO_FILE_IMPORT_H__
#define __IO_FILE_IMPORT_H__

#include "utils/types.h"

#include <glib.h>

TYPEDEF_STRUCT (SupportedFile);

G_BEGIN_DECLS

/**
 * @addtogroup io
 *
 * @{
 */

#define FILE_IMPORT_TYPE (file_import_get_type ())
G_DECLARE_FINAL_TYPE (FileImport, file_import, Z, FILE_IMPORT, GObject);

typedef struct _FileImportInfo
{
  /** Track to import on, if any, or 0. */
  unsigned int track_name_hash;

  /** Track lane to import on, if any, or -1. */
  int lane;

  /** Position to import the data at, or 1.1.1.0. */
  Position pos;

  /** Track index to start the import at. */
  int track_idx;
} FileImportInfo;

FileImportInfo *
file_import_info_new (void);

FileImportInfo *
file_import_info_clone (const FileImportInfo * src);

void
file_import_info_free (FileImportInfo * self);

/**
 * An object used for importing files asynchronously.
 */
typedef struct _FileImport
{
  GObject parent_instance;

  /** File path. */
  char * filepath;

  /** Return value. */
  GPtrArray * regions;

  /**
   * Owner of this FileImport instance, set to the GTask.
   *
   * This is to ensure that the internal GTask instance doesn't
   * attempt to call the async ready callback if the owner
   * is already destroyed.
   */
  GObject * owner;

  /** Import info. */
  FileImportInfo * import_info;
} FileImport;

/**
 * Returns a new FileImport instance.
 */
FileImport *
file_import_new (const char * filepath, const FileImportInfo * import_nfo);

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

GPtrArray *
file_import_sync (FileImport * self, GError ** error);

/**
 * To be called by the provided GAsyncReadyCallback to retrieve
 * retun values and error details, passing the GAsyncResult
 * which was passed to the callback.
 *
 * @return A pointer array of regions. The caller is
 *   responsible for freeing the pointer array and the regions.
 */
GPtrArray *
file_import_finish (FileImport * self, GAsyncResult * result, GError ** error);

/**
 * @}
 */

G_END_DECLS

#endif
