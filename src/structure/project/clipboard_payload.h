// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <memory>

#include "structure/project/project_registry.h"

#include <nlohmann/json_fwd.hpp>

using namespace std::string_view_literals;

namespace zrythm::structure::project
{

struct FilterResult;

/**
 * @brief A self-contained snapshot of copied project objects (arranger
 * objects, tracks or plugins) for clipboard operations.
 *
 * The payload mirrors the project file's registry format: flat buckets
 * (ports, parameters, plugins, tracks, arrangerObjects, fileAudioSources)
 * containing every object reachable from the copied roots, the root UUIDs,
 * and free-form per-type metadata (paste positioning context).
 *
 * Paste flow: filtered_for_target() → with_regenerated_uuids() →
 * import_into(), then attach the imported roots to their new owners with
 * undoable commands.
 */
class ClipboardPayload
{
public:
  enum class Type : std::uint8_t
  {
    ArrangerObjects,
    Tracks,
    Plugins,
  };

  static constexpr auto kDocumentType = "ZrythmClipboard"sv;
  static constexpr int  kFormatVersion = 1;

  /** Metadata keys of arranger-object payloads: the pasted group's
   * earliest position in ticks, and the source lane index per pasted
   * lane-clip root. */
  static constexpr auto kAnchorTicksMetadataKey = "anchorTicks"sv;
  static constexpr auto kLaneIndicesMetadataKey = "laneIndices"sv;

  /** Prefix of the text form put on the OS clipboard. */
  static constexpr auto kTextPrefix = "ZRYTHM-CLIPBOARD:v1:"sv;

  /**
   * @brief Upper bound on the total clipboard text length (in UTF-16
   * code units).
   *
   * Clipboard text comes from other processes and is decoded on the UI
   * thread: texts past this bound are rejected in
   * decode_from_clipboard_text() before any copying or decoding work
   * (together with kMaxDecodedPayloadSize bounding the decompressed
   * size), which caps the worst-case decode cost.
   */
  static constexpr qsizetype kMaxClipboardTextLength = 16LL * 1024 * 1024;

  /** Copies duplicate the JSON state: payloads stay independent after
   * copying. */
  ClipboardPayload ();
  ClipboardPayload (const ClipboardPayload &other);
  ClipboardPayload (ClipboardPayload &&other) noexcept;
  ClipboardPayload &operator= (const ClipboardPayload &other);
  ClipboardPayload &operator= (ClipboardPayload &&other) noexcept;
  ~ClipboardPayload ();

  /**
   * @brief Builds a payload by collecting the transitive closure of @p roots.
   *
   * Every UUID reference found in the serialized JSON of each visited object
   * is followed and the referenced object included, except boundary
   * references that may point outside the owned subgraph (channel send
   * destinations, parameter modulation sources): those either refer to
   * objects that are roots themselves (which are included on their own), or
   * are kept as-is / cleared at paste time.
   *
   * @param source_project_id Identifier of the source project (used at paste
   * time to decide whether external references still resolve).
   * @param metadata Free-form per-type metadata.
   */
  static ClipboardPayload create (
    const ProjectRegistry    &registry,
    Type                      type,
    const std::vector<QUuid> &roots,
    const QString            &source_project_id,
    nlohmann::json            metadata);

  /** Same as above, with empty metadata. */
  static ClipboardPayload create (
    const ProjectRegistry    &registry,
    Type                      type,
    const std::vector<QUuid> &roots,
    const QString            &source_project_id);

  Type           type () const { return type_; }
  const QString &source_project_id () const { return source_project_id_; }
  const nlohmann::json  &metadata () const;
  const nlohmann::json  &registry_json () const;
  std::span<const QUuid> roots () const { return roots_; }

  /**
   * @brief Returns the IDs of the payload objects that must stay
   * registered for the given roots to work: their transitive closure,
   * plus every payload object the kept entries reference (boundary
   * references included, so references into non-pasted roots are not
   * severed).
   *
   * The complement of this set within the IDs returned by import_into()
   * is safe to delete when some roots end up not pasted.
   */
  [[nodiscard]] std::vector<QUuid>
  ids_needed_by_roots (const std::vector<QUuid> &roots) const;

  /**
   * @brief Whether every non-boundary UUID reference in the registry and
   * metadata resolves to an entry of this payload's own registry.
   *
   * Copied payloads always satisfy this (they carry the closure of what
   * they reference); a schema-valid payload that fails it was crafted or
   * corrupted, and importing it would create null-resolving references.
   * Boundary keys (see is_boundary_key()) are exempt: they may point at
   * objects outside the payload by design.
   */
  [[nodiscard]] bool references_resolve_internally () const;

  /**
   * @brief Serializes to the prefixed, zstd-compressed base64 text form put
   * on the OS clipboard.
   */
  QString encode_to_clipboard_text () const;

  /**
   * @brief Parses clipboard text produced by encode_to_clipboard_text().
   *
   * @return The payload, or std::nullopt if the text is not a valid payload.
   */
  static std::optional<ClipboardPayload>
  decode_from_clipboard_text (const QString &text);

  /**
   * @brief Returns the payload with every object UUID replaced by a fresh
   * one.
   *
   * FileAudioSource UUIDs are kept: audio clips reference shared pooled
   * assets rather than owning them.
   *
   * Takes the payload by value: pass a temporary or std::move an lvalue
   * to avoid copying the registry JSON.
   */
  static ClipboardPayload with_regenerated_uuids (ClipboardPayload payload);

  /**
   * @brief Deletes the objects this payload imported from @p registry
   * (failed-import rollback).
   *
   * Objects no longer registered are skipped, so a parents-first cascade
   * that already removed an entry is harmless.
   */
  void cleanup_failed_import (
    ProjectRegistry       &registry,
    std::span<const QUuid> imported_ids) const;

  /**
   * @brief Returns a copy with content that cannot resolve in @p
   * target_registry removed, or std::nullopt if nothing pasteable remains.
   *
   * Audio content whose file audio source is not registered in the
   * target is dropped and counted (the frames live in a pool entry the
   * target does not have: always the case cross-project; in the same
   * project it means the source was purged after the copy). External
   * references that don't resolve in the target (send destinations,
   * modulation sources) are severed.
   *
   * Resolvability is registry membership: references to objects deleted
   * undoably are kept (the target can still be restored by undo, matching
   * how references survive undoable deletions inside a project); only
   * references to objects no longer registered at all are severed.
   */
  FilterResult
  filtered_for_target (const ProjectRegistry &target_registry) const;

  /**
   * @brief Imports the payload's objects into @p registry (two-phase
   * deserialization).
   *
   * FileAudioSources already registered in the target are skipped (shared
   * assets). Plugin instantiation begins during import and may finish
   * asynchronously: callers must wait for each imported plugin's
   * plugins::Plugin::InstantiationStatus to leave Pending before finalizing
   * the paste.
   *
   * @return The IDs of the objects newly registered in @p registry.
   * @throw ZrythmException on error; objects registered by the failed
   * import are cleaned up.
   */
  std::vector<QUuid> import_into (ProjectRegistry &registry) const;

  friend void to_json (nlohmann::json &j, const ClipboardPayload &payload);
  friend void from_json (const nlohmann::json &j, ClipboardPayload &payload);

private:
  /** Metadata and registry JSON; defined in the source file to keep the
   * full JSON header out of this one. */
  struct JsonState;
  std::unique_ptr<JsonState> json_;

  Type               type_{};
  QString            source_project_id_;
  std::vector<QUuid> roots_;
};

/**
 * @brief What ClipboardPayload::filtered_for_target() removed from a
 * payload.
 */
struct FilterResult
{
  /** The filtered payload, or std::nullopt if nothing pasteable remains. */
  std::optional<ClipboardPayload> payload;
  /** Pool-bound objects dropped (AudioClips and AudioSourceObjects)
   * because the target project cannot resolve their audio data. */
  std::size_t dropped_audio_objects = 0;
  /** External references that did not resolve in the target and were
   * severed (send destinations erased, modulation sources cleared). */
  std::size_t severed_references = 0;
};

} // namespace zrythm::structure::project
