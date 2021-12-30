#
#   This file is part of m.css.
#
#   Copyright © 2017, 2018, 2019, 2020 Vladimír Vondruš <mosra@centrum.cz>
#
#   Permission is hereby granted, free of charge, to any person obtaining a
#   copy of this software and associated documentation files (the "Software"),
#   to deal in the Software without restriction, including without limitation
#   the rights to use, copy, modify, merge, publish, distribute, sublicense,
#   and/or sell copies of the Software, and to permit persons to whom the
#   Software is furnished to do so, subject to the following conditions:
#
#   The above copyright notice and this permission notice shall be included
#   in all copies or substantial portions of the Software.
#
#   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
#   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
#   DEALINGS IN THE SOFTWARE.
#

# Can't be in __init__.py because I can't say `from . import Trie` in
# doxygen.py. But `from _search import bla` works. Ugh.

import base64
import enum
import struct
from types import SimpleNamespace as Empty
from typing import List, Tuple

# Version 0 was without the type map
searchdata_format_version = 1
search_filename = f'search-v{searchdata_format_version}.js'
searchdata_filename = f'searchdata-v{searchdata_format_version}.bin'
searchdata_filename_b85 = f'searchdata-v{searchdata_format_version}.js'

class CssClass(enum.Enum):
    DEFAULT = 0
    PRIMARY = 1
    SUCCESS = 2
    WARNING = 3
    DANGER = 4
    INFO = 5
    DIM = 6

class ResultFlag(enum.Flag):
    @staticmethod
    def from_type(flag: 'ResultFlag', type) -> 'ResultFlag':
        assert not flag & ResultFlag._TYPE
        assert type.value > 0 and type.value <= 0xf
        return flag|ResultFlag(type.value << 4)

    @property
    def type(self):
        return (int(self.value) >> 4) & 0xf

    NONE = 0

    HAS_SUFFIX = 1 << 0
    HAS_PREFIX = 1 << 3
    DEPRECATED = 1 << 1
    DELETED = 1 << 2 # TODO: this is C++-specific, put aside as well?

    # Result type. Order defines order in which equally-named symbols appear in
    # search results, every backend supplies its own, ranging from 1 << 4 to
    # 15 << 4.
    _TYPE = 0xf << 4
    ALIAS = 0 << 4 # This one gets the type from the referenced result

    # Otherwise it says "32 is not a valid ResultFlag"
    _TYPE01 = 1 << 4
    _TYPE02 = 2 << 4
    _TYPE03 = 3 << 4
    _TYPE04 = 4 << 4
    _TYPE05 = 5 << 4
    _TYPE06 = 6 << 4
    _TYPE07 = 7 << 4
    _TYPE08 = 8 << 4
    _TYPE09 = 9 << 4
    _TYPE10 = 10 << 4
    _TYPE11 = 11 << 4
    _TYPE12 = 12 << 4
    _TYPE13 = 13 << 4
    _TYPE14 = 14 << 4
    _TYPE15 = 15 << 4

class ResultMap:
    # item 1 flags | item 2 flags |     | item N flags | file | item 1 |
    #   + offset   |   + offset   | ... |   + offset   | size |  data  | ...
    #    8 + 24b   |    8 + 24b   |     |    8 + 24b   |  32b |        |
    #
    # basic item (flags & 0b11 == 0b00):
    #
    # name | \0 | URL
    #      |    |
    #      | 8b |
    #
    # suffixed item (flags & 0b11 == 0b01):
    #
    # suffix | name | \0 | URL
    # length |      |    |
    #   8b   |      | 8b |
    #
    # prefixed item (flags & 0xb11 == 0b10):
    #
    #  prefix  |  name  | \0 |  URL
    # id + len | suffix |    | suffix
    # 16b + 8b |        | 8b |
    #
    # prefixed & suffixed item (flags & 0xb11 == 0b11):
    #
    #  prefix  | suffix |  name  | \0 | URL
    # id + len | length | suffix |    |
    # 16b + 8b |   8b   |        | 8b |
    #
    # alias item (flags & 0xf0 == 0x00):
    #
    # alias |     | alias
    #  id   | ... | name
    #  16b  |     |
    #
    offset_struct = struct.Struct('<I')
    flags_struct = struct.Struct('<B')
    prefix_struct = struct.Struct('<HB')
    suffix_length_struct = struct.Struct('<B')
    alias_struct = struct.Struct('<H')

    def __init__(self):
        self.entries = []

    def add(self, name, url, alias=None, suffix_length=0, flags=ResultFlag(0)) -> int:
        if suffix_length: flags |= ResultFlag.HAS_SUFFIX
        if alias is not None:
            assert flags & ResultFlag._TYPE == ResultFlag.ALIAS

        entry = Empty()
        entry.name = name
        entry.url = url
        entry.flags = flags
        entry.alias = alias
        entry.prefix = 0
        entry.prefix_length = 0
        entry.suffix_length = suffix_length

        self.entries += [entry]
        return len(self.entries) - 1

    def serialize(self, merge_prefixes=True) -> bytearray:
        output = bytearray()

        if merge_prefixes:
            # Put all entry names into a trie to discover common prefixes
            trie = Trie()
            for index, e in enumerate(self.entries):
                trie.insert(e.name, index)

            # Create a new list with merged prefixes
            merged = []
            for index, e in enumerate(self.entries):
                # Search in the trie and get the longest shared name prefix
                # that is already fully contained in some other entry
                current = trie
                longest_prefix = None
                for c in e.name.encode('utf-8'):
                    for candidate, child in current.children.items():
                        if c == candidate:
                            current = child[1]
                            break
                    else: assert False # pragma: no cover

                    # Allow self-reference only when referenced result suffix
                    # is longer (otherwise cycles happen). This is for
                    # functions that should appear when searching for foo (so
                    # they get ordered properly based on the name length) and
                    # also when searching for foo() (so everything that's not
                    # a function gets filtered out). Such entries are
                    # completely the same except for a different suffix length.
                    if index in current.results:
                        for i in current.results:
                            if self.entries[i].suffix_length > self.entries[index].suffix_length:
                                longest_prefix = current
                                break
                    elif current.results:
                        longest_prefix = current

                # Name prefix found, for all possible URLs find the one that
                # shares the longest prefix
                if longest_prefix:
                    max_prefix = (0, -1)
                    for longest_index in longest_prefix.results:
                        # Ignore self (function self-reference, see above)
                        if longest_index == index: continue

                        prefix_length = 0
                        for i in range(min(len(e.url), len(self.entries[longest_index].url))):
                            if e.url[i] != self.entries[longest_index].url[i]: break
                            prefix_length += 1
                        if max_prefix[1] < prefix_length:
                            max_prefix = (longest_index, prefix_length)

                    # Expect we found something
                    assert max_prefix[1] != -1

                    # Save the entry with reference to the prefix
                    entry = Empty()
                    assert e.name.startswith(self.entries[longest_prefix.results[0]].name)
                    entry.name = e.name[len(self.entries[longest_prefix.results[0]].name):]
                    entry.url = e.url[max_prefix[1]:]
                    entry.flags = e.flags|ResultFlag.HAS_PREFIX
                    entry.alias = e.alias
                    entry.prefix = max_prefix[0]
                    entry.prefix_length = max_prefix[1]
                    entry.suffix_length = e.suffix_length
                    merged += [entry]

                # No prefix found, copy the entry verbatim
                else: merged += [e]

            # Everything merged, replace the original list
            self.entries = merged

        # Write the offset array. Starting offset for items is after the offset
        # array and the file size
        offset = (len(self.entries) + 1)*4
        for e in self.entries:
            assert offset < 2**24
            output += self.offset_struct.pack(offset)
            self.flags_struct.pack_into(output, len(output) - 1, e.flags.value)

            # The entry is an alias, extra field for alias index
            if e.flags & ResultFlag._TYPE == ResultFlag.ALIAS:
                offset += self.alias_struct.size

            # Extra field for prefix index and length
            if e.flags & ResultFlag.HAS_PREFIX:
                offset += self.prefix_struct.size

            # Extra field for suffix length
            if e.flags & ResultFlag.HAS_SUFFIX:
                offset += self.suffix_length_struct.size

            # Length of the name
            offset += len(e.name.encode('utf-8'))

            # Length of the URL and 0-delimiter. If URL is empty, it's not
            # added at all, then the 0-delimiter is also not needed.
            if e.name and e.url:
                 offset += len(e.url.encode('utf-8')) + 1

        # Write file size
        output += self.offset_struct.pack(offset)

        # Write the entries themselves
        for e in self.entries:
            if e.flags & ResultFlag._TYPE == ResultFlag.ALIAS:
                assert not e.alias is None
                assert not e.url
                output += self.alias_struct.pack(e.alias)
            if e.flags & ResultFlag.HAS_PREFIX:
                output += self.prefix_struct.pack(e.prefix, e.prefix_length)
            if e.flags & ResultFlag.HAS_SUFFIX:
                output += self.suffix_length_struct.pack(e.suffix_length)
            output += e.name.encode('utf-8')
            if e.url:
                output += b'\0'
                output += e.url.encode('utf-8')

        assert len(output) == offset
        return output

class Trie:
    #  root  |     |       header         | results | child 1 | child 1 | child 1 |
    # offset | ... | | result # | child # |   ...   |  char   | barrier | offset  | ...
    #  32b   |     |0|    7b    |   8b    |  n*16b  |   8b    |    1b   |   23b   |
    #
    # if result count > 127, it's instead:
    #
    #  root  |     |      header          | results | child 1 | child 1 | child 1 |
    # offset | ... | | result # | child # |   ...   |  char   | barrier | offset  | ...
    #  32b   |     |1|   11b    |   4b    |  n*16b  |   8b    |    1b   |   23b   |

    root_offset_struct = struct.Struct('<I')
    header_struct = struct.Struct('<BB')
    result_struct = struct.Struct('<H')
    child_struct = struct.Struct('<I')
    child_char_struct = struct.Struct('<B')

    def __init__(self):
        self.results = []
        self.children = {}

    def _insert(self, path: bytes, result, lookahead_barriers):
        if not path:
            self.results += [result]
            return

        char = path[0]
        if not char in self.children:
            self.children[char] = (False, Trie())
        if lookahead_barriers and lookahead_barriers[0] == 0:
            lookahead_barriers = lookahead_barriers[1:]
            self.children[char] = (True, self.children[char][1])
        self.children[char][1]._insert(path[1:], result, [b - 1 for b in lookahead_barriers])

    def insert(self, path: str, result, lookahead_barriers=[]):
        self._insert(path.encode('utf-8'), result, lookahead_barriers)

    def _sort(self, key):
        self.results.sort(key=key)
        for _, child in self.children.items():
            child[1]._sort(key)

    def sort(self, result_map: ResultMap):
        # What the shit, why can't I just take two elements and say which one
        # is in front of which, this is awful
        def key(item: int):
            entry = result_map.entries[item]
            return [
                # First order based on deprecation/deletion status, deprecated
                # always last, deleted in front of them, usable stuff on top
                2 if entry.flags & ResultFlag.DEPRECATED else 1 if entry.flags & ResultFlag.DELETED else 0,

                # Second order based on type (pages, then namespaces/classes,
                # later functions, values last)
                (entry.flags & ResultFlag._TYPE).value,

                # Third on suffix length (shortest first)
                entry.suffix_length,

                # Lastly on full name length (or prefix length, also shortest
                # first)
                len(entry.name)
            ]

        self._sort(key)

    # Returns offset of the serialized thing in `output`
    def _serialize(self, hashtable, output: bytearray, merge_subtrees) -> int:
        # Serialize all children first
        child_offsets = []
        for char, child in self.children.items():
            offset = child[1]._serialize(hashtable, output, merge_subtrees=merge_subtrees)
            child_offsets += [(char, child[0], offset)]

        # Serialize this node. Sometimes we'd have an insane amount of results
        # (such as Python's __init__), but very little children to go with
        # that. Then we can make the result count storage larger (11 bits,
        # 2048 results) and the child count storage smaller (4 bits, 16
        # children). Hopefully that's enough. The remaining leftmost bit is
        # used as an indicator of this shifted state.
        serialized = bytearray()
        if len(self.results) > 127:
            assert len(self.children) < 16 and len(self.results) < 2048
            result_count = (len(self.results) & 0x7f) | 0x80
            children_count = ((len(self.results) & 0xf80) >> 3) | len(self.children)
        else:
            result_count = len(self.results)
            children_count = len(self.children)
        serialized += self.header_struct.pack(result_count, children_count)
        for v in self.results:
            serialized += self.result_struct.pack(v)

        # Serialize child offsets
        for char, lookahead_barrier, abs_offset in child_offsets:
            assert abs_offset < 2**23

            # write them over each other because that's the only way to pack
            # a 24 bit field
            offset = len(serialized)
            serialized += self.child_struct.pack(abs_offset | ((1 if lookahead_barrier else 0) << 23))
            self.child_char_struct.pack_into(serialized, offset + 3, char)

        # Subtree merging: if this exact tree is already in the table, return
        # its offset. Otherwise add it and return the new offset.
        # TODO: why hashable = bytes(output[base_offset:] + serialized) didn't work?
        hashable = bytes(serialized)
        if merge_subtrees and hashable in hashtable:
            return hashtable[hashable]
        else:
            offset = len(output)
            output += serialized
            if merge_subtrees: hashtable[hashable] = offset
            return offset

    def serialize(self, merge_subtrees=True) -> bytearray:
        output = bytearray(b'\x00\x00\x00\x00')
        hashtable = {}
        self.root_offset_struct.pack_into(output, 0, self._serialize(hashtable, output, merge_subtrees=merge_subtrees))
        return output

#     type 1     |     type 2     |     |         |        | type 1 |
# class |  name  | class |  name  | ... | padding |  end   |  name  | ...
#   ID  | offset |   ID  | offset |     |         | offset |  data  |
#   8b  |   8b   |   8b  |   8b   |     |    8b   |   8b   |        |
type_map_entry_struct = struct.Struct('<BB')

def serialize_type_map(map: List[Tuple[CssClass, str]]) -> bytearray:
    serialized = bytearray()
    names = bytearray()

    # There's just 16 bits for the type and we're using one for aliases, so
    # that makes at most 15 values left. See ResultFlag for details.
    assert len(map) <= 15

    # Initial name offset is after all the offset entries plus the final one
    initial_name_offset = (len(map) + 1)*type_map_entry_struct.size

    # Add all entries (and the final offset), encode the names separately,
    # concatenate at the end
    for css_class, name in map:
        serialized += type_map_entry_struct.pack(css_class.value, initial_name_offset + len(names))
        names += name.encode('utf-8')
    serialized += type_map_entry_struct.pack(0, initial_name_offset + len(names))
    assert len(serialized) == initial_name_offset

    return serialized + names

# magic  | version | symbol | result |  type  |
# header |         | count  |  map   |  map   |
#        |         |        | offset | offset |
#  24b   |   8b    |  16b   |  32b   |  32b   |
search_data_header_struct = struct.Struct('<3sBHII')

def serialize_search_data(trie: Trie, map: ResultMap, type_map: List[Tuple[CssClass, str]], symbol_count, *, merge_subtrees=True, merge_prefixes=True) -> bytearray:
    serialized_trie = trie.serialize(merge_subtrees=merge_subtrees)
    serialized_map = map.serialize(merge_prefixes=merge_prefixes)
    serialized_type_map = serialize_type_map(type_map)

    preamble = search_data_header_struct.pack(b'MCS',
        searchdata_format_version, symbol_count,
        search_data_header_struct.size + len(serialized_trie),
        search_data_header_struct.size + len(serialized_trie) + len(serialized_map))
    return preamble + serialized_trie + serialized_map + serialized_type_map

def base85encode_search_data(data: bytearray) -> bytearray:
    return (b"/* Generated by https://mcss.mosra.cz/documentation/doxygen/. Do not edit. */\n" +
            b"Search.load('" + base64.b85encode(data, True) + b"');\n")

def _pretty_print_trie(serialized: bytearray, hashtable, stats, base_offset, indent, *, show_merged, show_lookahead_barriers, color_map) -> str:
    # Visualize where the trees were merged
    if show_merged and base_offset in hashtable:
        return color_map['red'] + '#' + color_map['reset']

    stats.node_count += 1

    out = ''
    result_count, child_count = Trie.header_struct.unpack_from(serialized, base_offset)
    stats.max_node_results = max(result_count, stats.max_node_results)
    stats.max_node_children = max(child_count, stats.max_node_children)
    offset = base_offset + Trie.header_struct.size

    # print results, if any
    if result_count:
        out += color_map['blue'] + ' ['
        for i in range(result_count):
            if i: out += color_map['blue']+', '
            result = Trie.result_struct.unpack_from(serialized, offset)[0]
            stats.max_node_result_index = max(result, stats.max_node_result_index)
            out += color_map['cyan'] + str(result)
            offset += Trie.result_struct.size
        out += color_map['blue'] + ']'

    # print children, if any
    for i in range(child_count):
        if result_count or i:
            out += color_map['reset'] + '\n'
            out += color_map['blue'] + indent + color_map['white']
        char = Trie.child_char_struct.unpack_from(serialized, offset + 3)[0]
        if char <= 127:
            out += chr(char)
        else:
            out += color_map['reset'] + hex(char)
        if (show_lookahead_barriers and Trie.child_struct.unpack_from(serialized, offset)[0] & 0x00800000):
            out += color_map['green'] + '$'
        if char > 127 or (show_lookahead_barriers and Trie.child_struct.unpack_from(serialized, offset)[0] & 0x00800000):
            out += color_map['reset'] + '\n' + color_map['blue'] + indent + ' ' + color_map['white']
        child_offset = Trie.child_struct.unpack_from(serialized, offset)[0] & 0x007fffff
        stats.max_node_child_offset = max(child_offset, stats.max_node_child_offset)
        offset += Trie.child_struct.size
        out += _pretty_print_trie(serialized, hashtable, stats, child_offset, indent + ('|' if child_count > 1 else ' '), show_merged=show_merged, show_lookahead_barriers=show_lookahead_barriers, color_map=color_map)
        child_count += 1

    hashtable[base_offset] = True
    return out

color_map_colors = {'blue': '\033[0;34m',
                    'white': '\033[1;39m',
                    'red': '\033[1;31m',
                    'green': '\033[1;32m',
                    'cyan': '\033[1;36m',
                    'yellow': '\033[1;33m',
                    'reset': '\033[0m'}

color_map_dummy = {'blue': '',
                   'white': '',
                   'red': '',
                   'green': '',
                   'cyan': '',
                   'yellow': '',
                   'reset': ''}

def pretty_print_trie(serialized: bytes, *, show_merged=False, show_lookahead_barriers=True, colors=False):
    color_map = color_map_colors if colors else color_map_dummy

    hashtable = {}

    stats = Empty()
    stats.node_count = 0
    stats.max_node_results = 0
    stats.max_node_children = 0
    stats.max_node_result_index = 0
    stats.max_node_child_offset = 0

    out = _pretty_print_trie(serialized, hashtable, stats, Trie.root_offset_struct.unpack_from(serialized, 0)[0], '', show_merged=show_merged, show_lookahead_barriers=show_lookahead_barriers, color_map=color_map)
    if out: out = color_map['white'] + out
    stats = """
node count:             {}
max node results:       {}
max node children:      {}
max node result index:  {}
max node child offset:  {}""".lstrip().format(stats.node_count, stats.max_node_results, stats.max_node_children, stats.max_node_result_index, stats.max_node_child_offset)
    return out, stats

def pretty_print_map(serialized: bytes, *, entryTypeClass, colors=False):
    color_map = color_map_colors if colors else color_map_dummy

    # The first item gives out offset of first value, which can be used to
    # calculate total value count
    offset = ResultMap.offset_struct.unpack_from(serialized, 0)[0] & 0x00ffffff
    size = int(offset/4 - 1)

    out = ''
    for i in range(size):
        if i: out += '\n'
        flags = ResultFlag(ResultMap.flags_struct.unpack_from(serialized, i*4 + 3)[0])
        extra = []
        if flags & ResultFlag._TYPE == ResultFlag.ALIAS:
            extra += ['alias={}'.format(ResultMap.alias_struct.unpack_from(serialized, offset)[0])]
            offset += ResultMap.alias_struct.size
        if flags & ResultFlag.HAS_PREFIX:
            extra += ['prefix={}[:{}]'.format(*ResultMap.prefix_struct.unpack_from(serialized, offset))]
            offset += ResultMap.prefix_struct.size
        if flags & ResultFlag.HAS_SUFFIX:
            extra += ['suffix_length={}'.format(ResultMap.suffix_length_struct.unpack_from(serialized, offset)[0])]
            offset += ResultMap.suffix_length_struct.size
        if flags & ResultFlag.DEPRECATED:
            extra += ['deprecated']
        if flags & ResultFlag.DELETED:
            extra += ['deleted']
        if flags & ResultFlag._TYPE:
            extra += ['type={}'.format(entryTypeClass(flags.type).name)]
        next_offset = ResultMap.offset_struct.unpack_from(serialized, (i + 1)*4)[0] & 0x00ffffff
        name, _, url = serialized[offset:next_offset].partition(b'\0')
        out += color_map['cyan'] + str(i) + color_map['blue'] + ': ' + color_map['white'] + name.decode('utf-8') + color_map['blue'] + ' [' + color_map['yellow'] + (color_map['blue'] + ', ' + color_map['yellow']).join(extra) + color_map['blue'] + '] ->' + (' ' + color_map['reset'] + url.decode('utf-8') if url else '')
        offset = next_offset
    return out

def pretty_print_type_map(serialized: bytes, *, entryTypeClass):
    # Unpack until we aren't at EOF
    i = 0
    out = ''
    class_id, offset = type_map_entry_struct.unpack_from(serialized, 0)
    while offset < len(serialized):
        if i: out += ',\n'
        next_class_id, next_offset = type_map_entry_struct.unpack_from(serialized, (i + 1)*type_map_entry_struct.size)
        out += "({}, {}, '{}')".format(entryTypeClass(i + 1), CssClass(class_id), serialized[offset:next_offset].decode('utf-8'))
        i += 1
        class_id, offset = next_class_id, next_offset
    return out

def pretty_print(serialized: bytes, *, entryTypeClass, show_merged=False, show_lookahead_barriers=True, colors=False):
    magic, version, symbol_count, map_offset, type_map_offset = search_data_header_struct.unpack_from(serialized)
    assert magic == b'MCS'
    assert version == searchdata_format_version

    pretty_trie, stats = pretty_print_trie(serialized[search_data_header_struct.size:map_offset], show_merged=show_merged, show_lookahead_barriers=show_lookahead_barriers, colors=colors)
    pretty_map = pretty_print_map(serialized[map_offset:type_map_offset], entryTypeClass=entryTypeClass, colors=colors)
    pretty_type_map = pretty_print_type_map(serialized[type_map_offset:], entryTypeClass=entryTypeClass)
    return '{} symbols\n'.format(symbol_count) + pretty_trie + '\n' + pretty_map + '\n' + pretty_type_map, stats
