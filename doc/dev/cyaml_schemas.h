// SPDX-FileCopyrightText: © 2019 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 *
 * \page cyaml_schemas Cyaml Schemas
 *
 * \section introduction_cyaml Introduction
 *
 * Zrythm uses <a href="https://github.com/tlsa/libcyaml">libcyaml</a> to save projects into YAML.
 * Cyaml can serialize C structs, given that you
 * provide it with the schemas of each struct. This
 * page has some tips on writing schemas.
 *
 * @note Some examples are no longer in the
 * code base and are used only for demonstration
 * purposes
 *
 * \section fields Fields
 *
 * For all fields, use the
 * @code CYAML_FLAG_OPTIONAL @endcode flag to
 * indicate that a variable can be NULL.
 *
 * \subsection int_field Ints
 *
 * Use @code CYAML_FIELD_INT @endcode with the
 * @code CYAML_FLAG_DEFAULT @endcode flag.
 *
 * Example, having an 'enabled' flag inside a
 *   Plugin:
 * @code
 * CYAML_FIELD_INT (
 *   "enabled", CYAML_FLAG_DEFAULT,
 *   Plugin, enabled),
 * @endcode
 *
 * \subsection enum_field Enums
 *
 * Use @code CYAML_FIELD_ENUM @endcode with the
 * @code CYAML_FLAG_DEFAULT @endcode flag.
 *
 * Example, RegionType inside a Region:
 * @code
 * CYAML_FIELD_ENUM (
 *   "type", CYAML_FLAG_DEFAULT,
 *   Region, type, region_type_strings,
 *   CYAML_ARRAY_LEN (region_type_strings)),
 * @endcode
 *
 * Enums also need a cyaml_strval_t argument:
 * @code
 * static const cyaml_strval_t
 * region_type_strings[] =
 * {
 *   { "Midi",          REGION_TYPE_MIDI    },
 *   { "Audio",         REGION_TYPE_AUDIO   },
 * };
 * @endcode
 *
 * \subsection string_field String Pointers (char *)
 *
 * Use @code CYAML_FIELD_STRING_PTR @endcode with the
 * @code CYAML_FLAG_POINTER @endcode flag.
 *
 * Example, Region name:
 * @code
 * CYAML_FIELD_STRING_PTR (
 *   "name", CYAML_FLAG_POINTER,
 *   Region, name,
 *  	0, CYAML_UNLIMITED),
 * @endcode
 *
 * \subsection struct_in_struct_mapping Direct struct (not pointer) inside another struct
 *
 * Use @code CYAML_FIELD_MAPPING @endcode with the
 * @code CYAML_FLAG_DEFAULT @endcode flag.
 *
 * Example, having a plain Position inside a
 *   MidiNote:
 * @code
 * CYAML_FIELD_MAPPING (
 *   "start_pos",
 *   CYAML_FLAG_DEFAULT,
 *   MidiNote, start_pos, position_fields_schema)
 * @endcode
 *
 * \subsection pointer_field Pointer to Struct
 *
 * Use @code CYAML_FIELD_MAPPING_PTR @endcode with the
 * @code CYAML_FLAG_POINTER @endcode flag.
 *
 * Example, having a pointer to an Lv2Plugin inside
 *   a Plugin:
 * @code
 * CYAML_FIELD_MAPPING_PTR (
 *   "lv2", CYAML_FLAG_POINTER,
 *   Plugin, lv2,
 *   lv2_plugin_fields_schema),
 * @endcode
 *
 * \subsection array_of_primitives Array of primitives
 *
 * For fixed-size arrays without a counter,
 * @code
 * CYAML_FIELD_SEQUENCE_FIXED (
 *   "midi_channels", CYAML_FLAG_DEFAULT,
 *   Channel, midi_channels,
 *   &int_schema, 16),
 * @endcode
 *
 * \subsection array_of_pointers_variable_field Arrays of Pointers (variable count)
 *
 * There are two cases. Fixed-width arrays,
 * like below. In this case,
 * use the @code CYAML_FLAG_DEFAULT @endcode flag.
 * @code
 * MyStruct * my_structs[MAX_STRUCTS];
 * int        num_my_structs;
 * @endcode
 *
 * The other case is dynamic arrays, like below.
 * In this case use @code @CYAML_FLAG_POINTER
 * @endcode.
 * @code
 * AutomationTrack ** ats;
 * int               num_ats;
 * int               ats_size;
 * @endcode
 *
 * Use @code CYAML_FIELD_SEQUENCE_COUNT @endcode with the
 * @code CYAML_FLAG_DEFAULT @endcode or @code
 * CYAML_FLAG_POINTER @endcode flag as described
 * above.
 *
 * Use @code CYAML_FLAG_POINTER @endcode when
 * declaring the schema of the child struct.
 *
 * Example, fixed-size array of MidiNote pointers inside a
 * Region:
 * @code
 * CYAML_FIELD_SEQUENCE_COUNT (
 *   "midi_notes", CYAML_FLAG_DEFAULT,
 *   Region, midi_notes, num_midi_notes,
 *   &midi_note_schema, 0, CYAML_UNLIMITED),
 * @endcode
 *
 * Example, dynamic array of AutomationTrack
 * pointers in AutomationTracklist:
 * @code
 * CYAML_FIELD_SEQUENCE_COUNT (
 *   "ats", CYAML_FLAG_POINTER,
 *   AutomationTracklist, ats, num_ats,
 *   &automation_track_schema, 0, CYAML_UNLIMITED),
 * @endcode
 *
 * \subsection arrays_of_pointers_fixed_field  Arrays of Pointers (fixed count)
 *
 * @code
 * MyStruct * my_structs[9];
 * @endcode
 *
 * Use @code CYAML_FIELD_SEQUENCE_FIXED @endcode
 * with the @code CYAML_FLAG_DEFAULT @endcode flag.
 *
 * Example:
 * @code
 * CYAML_FIELD_SEQUENCE_FIXED (
 *   "plugins", CYAML_FLAG_DEFAULT,
 *   Channel, plugins,
 *   &plugin_schema, STRIP_SIZE),
 * @endcode
*
 * \subsection array_of_structs_variable_field Arrays of Structs (variable count)
 *
 * @code
 * MyStruct my_structs[MAX_MY_STRUCTS];
 * int      num_my_structs;
 * @endcode
 *
 * Use @code CYAML_FIELD_SEQUENCE_COUNT @endcode with the
 * @code CYAML_FLAG_DEFAULT @endcode flag.
 *
 * Use @code CYAML_FLAG_DEFAULT @endcode when
 * declaring the schema of the child struct.
 *
 * Example, destination PortIdentifier's of a Port:
 * @code
 * static const cyaml_schema_value_t
 * port_identifier_schema_default = {
 *   CYAML_VALUE_MAPPING (
 *     CYAML_FLAG_DEFAULT, // note the flag
 *     PortIdentifier,
 *     port_identifier_fields_schema),
 * };
 *
 * CYAML_FIELD_SEQUENCE_COUNT (
 *   "dest_ids", CYAML_FLAG_DEFAULT,
 *   Port, dest_ids, num_dests,
 *   &port_identifier_schema_default,
 *   0, CYAML_UNLIMITED),
 * @endcode
 *
 * \subsection cyaml_bitfields Bitfields
 *
 * @code
 * typedef enum PortFlags
 * {
 *   PORT_FLAG_STEREO_L = 0x01,
 *   PORT_FLAG_STEREO_R = 0x02,
 *   PORT_FLAG_PIANO_ROLL = 0x04,
 *   PORT_FLAG_SIDECHAIN = 0x08,
 *   PORT_FLAG_MAIN_PORT = 0x10,
 *   OPT_F = 0x20,
 * } PortFlags;
 * @endcode
 *
 * First, declare the bitfield definitions.
 *
 * @code
 * static const cyaml_bitdef_t
 * flags_bitvals[] =
 * {
 *   { .name = "stereo_l", .offset =  0, .bits =  1 },
 *   { .name = "stereo_r", .offset =  1, .bits =  1 },
 *   { .name = "piano_roll", .offset = 2, .bits = 1 },
 *   { .name = "sidechain", .offset = 3, .bits =  1 },
 *   { .name = "main_port", .offset = 4, .bits = 1 },
 *   { .name = "opt_f", .offset = 5, .bits = 1 },
 * };
 * @endcode
 *
 * Use @code CYAML_FIELD_BITFIELD @endcode with the
 * @code CYAML_FLAG_DEFAULT @endcode flag.
 *
 * Example, PortFlags:
 * @code
 * CYAML_FIELD_BITFIELD (
 *   "flags", CYAML_FLAG_DEFAULT,
 *   PortIdentifier, flags, flags_bitvals,
 *   CYAML_ARRAY_LEN (flags_bitvals)),
 * @endcode
 *
 * \section cyaml_schema_values Schema Values
 *
 * Schemas will normally be @code CYAML_VALUE_MAPPING
 * @endcode with a @code CYAML_FLAG_POINTER @endcode
 * flag. Use the @code CYAML_FLAG_DEFAULT @endcode
 * if the struct will be used directly in another
 * struct (not as a pointer).
 */
