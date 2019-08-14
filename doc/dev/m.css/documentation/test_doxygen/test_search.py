#!/usr/bin/env python3

#
#   This file is part of m.css.
#
#   Copyright © 2017, 2018, 2019 Vladimír Vondruš <mosra@centrum.cz>
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

import argparse
import os
import sys
import unittest
from types import SimpleNamespace as Empty

from doxygen import Trie, ResultMap, ResultFlag, serialize_search_data, search_data_header_struct

from test_doxygen import IntegrationTestCase

def _pretty_print_trie(serialized: bytearray, hashtable, stats, base_offset, indent, show_merged, show_lookahead_barriers, color_map) -> str:
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

def pretty_print_trie(serialized: bytes, show_merged=False, show_lookahead_barriers=True, colors=False):
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

def pretty_print_map(serialized: bytes, colors=False):
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
            extra += ['type={}'.format((flags & ResultFlag._TYPE).name)]
        next_offset = ResultMap.offset_struct.unpack_from(serialized, (i + 1)*4)[0] & 0x00ffffff
        name, _, url = serialized[offset:next_offset].partition(b'\0')
        out += color_map['cyan'] + str(i) + color_map['blue'] + ': ' + color_map['white'] + name.decode('utf-8') + color_map['blue'] + ' [' + color_map['yellow'] + (color_map['blue'] + ', ' + color_map['yellow']).join(extra) + color_map['blue'] + '] ->' + (' ' + color_map['reset'] + url.decode('utf-8') if url else '')
        offset = next_offset
    return out

def pretty_print(serialized: bytes, show_merged=False, show_lookahead_barriers=True, colors=False):
    magic, version, symbol_count, map_offset = search_data_header_struct.unpack_from(serialized)
    assert magic == b'MCS'
    assert version == 0

    pretty_trie, stats = pretty_print_trie(serialized[search_data_header_struct.size:map_offset], show_merged=show_merged, show_lookahead_barriers=show_lookahead_barriers, colors=colors)
    pretty_map = pretty_print_map(serialized[map_offset:], colors=colors)
    return '{} symbols\n'.format(symbol_count) + pretty_trie + '\n' + pretty_map, stats

class TrieSerialization(unittest.TestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.maxDiff = None

    def compare(self, serialized: bytes, expected: str):
        pretty = pretty_print_trie(serialized)[0]
        #print(pretty)
        self.assertEqual(pretty, expected.strip())

    def test_empty(self):
        trie = Trie()

        serialized = trie.serialize()
        self.compare(serialized, "")
        self.assertEqual(len(serialized), 6)

    def test_single(self):
        trie = Trie()
        trie.insert("magnum", 1337)
        trie.insert("magnum", 21)

        serialized = trie.serialize()
        self.compare(serialized, """
magnum [1337, 21]
""")
        self.assertEqual(len(serialized), 46)

    def test_multiple(self):
        trie = Trie()

        trie.insert("math", 0)
        trie.insert("math::vector", 1, lookahead_barriers=[4])
        trie.insert("vector", 1)
        trie.insert("math::range", 2)
        trie.insert("range", 2)

        trie.insert("math::min", 3)
        trie.insert("min", 3)
        trie.insert("math::max", 4)
        trie.insert("max", 4)
        trie.insert("math::minmax", 5)
        trie.insert("minmax", 5)

        trie.insert("math::vector::minmax", 6, lookahead_barriers=[4, 12])
        trie.insert("vector::minmax", 6, lookahead_barriers=[6])
        trie.insert("minmax", 6)
        trie.insert("math::vector::min", 7)
        trie.insert("vector::min", 7)
        trie.insert("min", 7)
        trie.insert("math::vector::max", 8)
        trie.insert("vector::max", 8)
        trie.insert("max", 8)

        trie.insert("math::range::min", 9, lookahead_barriers=[4, 11])
        trie.insert("range::min", 9, lookahead_barriers=[5])
        trie.insert("min", 9)

        trie.insert("math::range::max", 10)
        trie.insert("range::max", 10)
        trie.insert("max", 10)

        serialized = trie.serialize()
        self.compare(serialized, """
math [0]
||| :$
|||  :vector [1]
|||   |     :$
|||   |      :min [7]
|||   |        | max [6]
|||   |        ax [8]
|||   range [2]
|||   |    :$
|||   |     :min [9]
|||   |       ax [10]
|||   min [3]
|||   || max [5]
|||   |ax [4]
||x [4, 8, 10]
|in [3, 7, 9]
|| max [5, 6]
vector [1]
|     :$
|      :min [7]
|        | max [6]
|        ax [8]
range [2]
|    :$
|     :min [9]
|       ax [10]
""")
        self.assertEqual(len(serialized), 340)

    def test_unicode(self):
        trie = Trie()

        trie.insert("hýždě", 0)
        trie.insert("hárá", 1)

        serialized = trie.serialize()
        self.compare(serialized, """
h0xc3
  0xbd
   0xc5
  | 0xbe
  |  d0xc4
  |    0x9b
  |      [0]
  0xa1
   r0xc3
  |  0xa1
  |    [1]
""")
        self.assertEqual(len(serialized), 82)

class MapSerialization(unittest.TestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.maxDiff = None

    def compare(self, serialized: bytes, expected: str):
        pretty = pretty_print_map(serialized)
        #print(pretty)
        self.assertEqual(pretty, expected.strip())

    def test_empty(self):
        map = ResultMap()

        serialized = map.serialize()
        self.compare(serialized, "")
        self.assertEqual(len(serialized), 4)

    def test_single(self):
        map = ResultMap()
        self.assertEqual(map.add("Magnum", "namespaceMagnum.html", suffix_length=11, flags=ResultFlag.NAMESPACE), 0)

        serialized = map.serialize()
        self.compare(serialized, """
0: Magnum [suffix_length=11, type=NAMESPACE] -> namespaceMagnum.html
""")
        self.assertEqual(len(serialized), 36)

    def test_multiple(self):
        map = ResultMap()

        self.assertEqual(map.add("Math", "namespaceMath.html", flags=ResultFlag.NAMESPACE), 0)
        self.assertEqual(map.add("Math::Vector", "classMath_1_1Vector.html", flags=ResultFlag.CLASS), 1)
        self.assertEqual(map.add("Math::Range", "classMath_1_1Range.html", flags=ResultFlag.CLASS), 2)
        self.assertEqual(map.add("Math::min()", "namespaceMath.html#abcdef2875", flags=ResultFlag.FUNC), 3)
        self.assertEqual(map.add("Math::max(int, int)", "namespaceMath.html#abcdef1234", suffix_length=8, flags=ResultFlag.FUNC|ResultFlag.DEPRECATED|ResultFlag.DELETED), 4)
        self.assertEqual(map.add("Rectangle", "", alias=2), 5)
        self.assertEqual(map.add("Rectangle::Rect()", "", suffix_length=2, alias=2), 6)

        serialized = map.serialize()
        self.compare(serialized, """
0: Math [type=NAMESPACE] -> namespaceMath.html
1: ::Vector [prefix=0[:0], type=CLASS] -> classMath_1_1Vector.html
2: ::Range [prefix=0[:0], type=CLASS] -> classMath_1_1Range.html
3: ::min() [prefix=0[:18], type=FUNC] -> #abcdef2875
4: ::max(int, int) [prefix=0[:18], suffix_length=8, deprecated, deleted, type=FUNC] -> #abcdef1234
5: Rectangle [alias=2] ->
6: ::Rect() [alias=2, prefix=5[:0], suffix_length=2] ->
""")
        self.assertEqual(len(serialized), 203)

class Serialization(unittest.TestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.maxDiff = None

    def compare(self, serialized: bytes, expected: str):
        pretty = pretty_print(serialized)[0]
        #print(pretty)
        self.assertEqual(pretty, expected.strip())

    def test(self):
        trie = Trie()
        map = ResultMap()

        trie.insert("math", map.add("Math", "namespaceMath.html", flags=ResultFlag.NAMESPACE))
        index = map.add("Math::Vector", "classMath_1_1Vector.html", flags=ResultFlag.CLASS)
        trie.insert("math::vector", index)
        trie.insert("vector", index)
        index = map.add("Math::Range", "classMath_1_1Range.html", flags=ResultFlag.CLASS)
        trie.insert("math::range", index)
        trie.insert("range", index)

        serialized = serialize_search_data(trie, map, 3)
        self.compare(serialized, """
3 symbols
math [0]
|   ::vector [1]
|     range [2]
vector [1]
range [2]
0: Math [type=NAMESPACE] -> namespaceMath.html
1: ::Vector [prefix=0[:0], type=CLASS] -> classMath_1_1Vector.html
2: ::Range [prefix=0[:0], type=CLASS] -> classMath_1_1Range.html
""")
        self.assertEqual(len(serialized), 241)

class Search(IntegrationTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, '', *args, **kwargs)

    def test(self):
        self.run_doxygen(index_pages=[], wildcard='*.xml')

        with open(os.path.join(self.path, 'html', 'searchdata.bin'), 'rb') as f:
            serialized = f.read()
            search_data_pretty = pretty_print(serialized)[0]
        #print(search_data_pretty)
        self.assertEqual(len(serialized), 4695)
        self.assertEqual(search_data_pretty, """
53 symbols
deprecated_macro [0]
||        |     ($
||        |      ) [1]
||        dir [23]
||        |  /$
||        |   deprecatedfile.h [2]
||        file.h [2]
||        |oo [37]
||        || ($
||        ||  ) [38]
||         list [22]
||        namespace [39]
||        |        :$
||        |         :deprecatedenum [32]
||        |          |         |   :$
||        |          |         |    :value [31]
||        |          |         typedef [35]
||        |          |         variable [36]
||        |          |         foo [37]
||        |          |         |  ($
||        |          |         |   ) [38]
||        |          |         class [53]
||        |          |         struct [54]
||        |          |         union [58]
||        |          enum [34]
||        |          |   :$
||        |          |    :deprecatedvalue [33]
||        enum [32]
||        |   :$
||        |    :value [31]
||        value [33]
||        | riable [36]
||        typedef [35]
||        class [53]
||        struct [54]
||        union [58]
|ir [24]
|| /$
||  file.h [9]
macro [3]
||   _function [5]
||            ($
||             ) [6]
||            _with_params [7]
||            |           ($
||            |            ) [8]
|in() [49]
glmacro() [4]
| file() [10]
| |oo() [13]
| class() [21]
| directory() [25]
| |  () [26]
| _dir [27]
| |group [30]
| |enum [45]
| ||   _value [43]
| ||         _ext [42]
| |typedef [47]
| namespace() [51]
| struct() [56]
| union() [60]
file.h [9]
|oo [11, 14, 18, 16]
|| ($
||  ) [12, 15, 19, 17]
namespace [50]
|        :$
|         :class [20]
|          |    :$
|          |     :foo [11, 14, 18, 16]
|          |         ($
|          |          ) [12, 15, 19, 17]
|          enum [44]
|          |   :$
|          |    :onlyabrief [40]
|          |     value [41]
|          typedef [46]
|          variable [48]
|          struct [55]
|          union [59]
class [20]
|    :$
|     :foo [11, 14, 18, 16]
|         ($
|          ) [12, 15, 19, 17]
a group [29, 28]
| page [52]
value [41, 31]
| riable [48]
enum [44, 34]
|   :$
|    :deprecatedvalue [33]
|     onlyabrief [40]
|     value [41]
onlyabrief [40]
typedef [46]
struct [55]
|ubpage [57]
union [59]
0: DEPRECATED_MACRO(a, b, c) [suffix_length=9, deprecated, type=DEFINE] -> DeprecatedFile_8h.html#a7f8376730349fef9ff7d103b0245a13e
1:  [prefix=0[:56], suffix_length=7, deprecated, type=DEFINE] ->
2: /DeprecatedFile.h [prefix=23[:0], deprecated, type=FILE] -> DeprecatedFile_8h.html
3: MACRO [type=DEFINE] -> File_8h.html#a824c99cb152a3c2e9111a2cb9c34891e
4: glMacro() [alias=3] ->
5: _FUNCTION() [prefix=3[:14], suffix_length=2, type=DEFINE] -> 025158d6007b306645a8eb7c7a9237c1
6:  [prefix=5[:46], type=DEFINE] ->
7: _FUNCTION_WITH_PARAMS(params) [prefix=3[:15], suffix_length=8, type=DEFINE] -> 8602bba5a72becb4f2dc544ce12c420
8:  [prefix=7[:46], suffix_length=6, type=DEFINE] ->
9: /File.h [prefix=24[:0], type=FILE] -> File_8h.html
10: glFile() [alias=9] ->
11: ::foo() [prefix=20[:28], suffix_length=2, type=FUNC] -> #aaeba4096356215868370d6ea476bf5d9
12:  [prefix=11[:62], type=FUNC] ->
13: glFoo() [alias=11] ->
14:  const [prefix=11[:30], suffix_length=8, type=FUNC] -> c03c5b93907dda16763eabd26b25500a
15:  [prefix=14[:62], suffix_length=6, type=FUNC] ->
16:  && [prefix=11[:30], suffix_length=5, deleted, type=FUNC] -> 77803233441965cad057a6619e9a75fd
17:  [prefix=16[:62], suffix_length=3, deleted, type=FUNC] ->
18: ::foo(const Enum&, Typedef) [prefix=20[:28], suffix_length=22, type=FUNC] -> #aba8d57a830d4d79f86d58d92298677fa
19:  [prefix=18[:62], suffix_length=20, type=FUNC] ->
20: ::Class [prefix=50[:0], type=CLASS] -> classNamespace_1_1Class.html
21: glClass() [alias=20] ->
22: Deprecated List [type=PAGE] -> deprecated.html
23: DeprecatedDir [deprecated, type=DIR] -> dir_c6c97faf5a6cbd0f62c27843ce3af4d0.html
24: Dir [type=DIR] -> dir_da5033def2d0db76e9883b31b76b3d0c.html
25: glDirectory() [alias=24] ->
26: glDir() [alias=24] ->
27: GL_DIR [alias=24] ->
28: A group [deprecated, type=GROUP] -> group__deprecated-group.html
29: A group [type=GROUP] -> group__group.html
30: GL_GROUP [alias=29] ->
31: ::Value [prefix=32[:67], type=ENUM_VALUE] -> a689202409e48743b914713f96d93947c
32: ::DeprecatedEnum [prefix=39[:33], deprecated, type=ENUM] -> #ab1e37ddc1d65765f2a48485df4af7b47
33: ::DeprecatedValue [prefix=34[:67], deprecated, type=ENUM_VALUE] -> a4b5b0e9709902228c33df7e5e377e596
34: ::Enum [prefix=39[:33], type=ENUM] -> #ac59010e983270c330b8625b5433961b9
35: ::DeprecatedTypedef [prefix=39[:33], deprecated, type=TYPEDEF] -> #af503ad3ff194a4c2512aff16df771164
36: ::DeprecatedVariable [prefix=39[:33], deprecated, type=VAR] -> #ae934297fc39624409333eefbfeabf5e5
37: ::deprecatedFoo(int, bool, double) [prefix=39[:33], suffix_length=19, deprecated, type=FUNC] -> #a9a1b3fc71d294b548095985acc0d5092
38:  [prefix=37[:67], suffix_length=17, deprecated, type=FUNC] ->
39: DeprecatedNamespace [deprecated, type=NAMESPACE] -> namespaceDeprecatedNamespace.html
40: ::OnlyABrief [prefix=44[:57], type=ENUM_VALUE] -> a9b0246417d89d650ed429f1b784805eb
41: ::Value [prefix=44[:57], type=ENUM_VALUE] -> a689202409e48743b914713f96d93947c
42: _EXT [alias=41, prefix=43[:0]] ->
43: _VALUE [alias=41, prefix=45[:0]] ->
44: ::Enum [prefix=50[:23], type=ENUM] -> #add172b93283b1ab7612c3ca6cc5dcfea
45: GL_ENUM [alias=44] ->
46: ::Typedef [prefix=50[:23], type=TYPEDEF] -> #abe2a245304bc2234927ef33175646e08
47: GL_TYPEDEF [alias=46] ->
48: ::Variable [prefix=50[:23], type=VAR] -> #ad3121960d8665ab045ca1bfa1480a86d
49: GLSL: min() [alias=48, suffix_length=2] ->
50: Namespace [type=NAMESPACE] -> namespaceNamespace.html
51: glNamespace() [alias=50] ->
52: A page [type=PAGE] -> page.html
53: ::DeprecatedClass [prefix=39[:0], deprecated, type=STRUCT] -> structDeprecatedNamespace_1_1DeprecatedClass.html
54: ::DeprecatedStruct [prefix=39[:0], deprecated, type=STRUCT] -> structDeprecatedNamespace_1_1DeprecatedStruct.html
55: ::Struct [prefix=50[:0], type=STRUCT] -> structNamespace_1_1Struct.html
56: glStruct() [alias=55] ->
57:  » Subpage [prefix=52[:0], type=PAGE] -> subpage.html
58: ::DeprecatedUnion [prefix=39[:0], deprecated, type=UNION] -> unionDeprecatedNamespace_1_1DeprecatedUnion.html
59: ::Union [prefix=50[:0], type=UNION] -> unionNamespace_1_1Union.html
60: glUnion() [alias=59] ->
""".strip())

class SearchLongSuffixLength(IntegrationTestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(__file__, 'long_suffix_length', *args, **kwargs)

    def test(self):
        self.run_doxygen(index_pages=[], wildcard='*.xml')

        with open(os.path.join(self.path, 'html', 'searchdata.bin'), 'rb') as f:
            serialized = f.read()
            search_data_pretty = pretty_print(serialized)[0]
        #print(search_data_pretty)
        self.assertEqual(len(serialized), 382)
        # The parameters get cut off with an ellipsis
        self.assertEqual(search_data_pretty, """
2 symbols
file.h [2]
|     :$
|      :averylongfunctionname [0]
|                            ($
|                             ) [1]
averylongfunctionname [0]
|                    ($
|                     ) [1]
0: ::aVeryLongFunctionName(const std::reference_wrapper<const std::vector<std::string>>&, c…) [prefix=2[:12], suffix_length=69, type=FUNC] -> #a1e9a11887275938ef5541070955c9d9c
1:  [prefix=0[:46], suffix_length=67, type=FUNC] ->
2: File.h [type=FILE] -> File_8h.html
""".strip())

if __name__ == '__main__': # pragma: no cover
    parser = argparse.ArgumentParser()
    parser.add_argument('file', help="file to pretty-print")
    parser.add_argument('--show-merged', help="show merged subtrees", action='store_true')
    parser.add_argument('--show-lookahead-barriers', help="show lookahead barriers", action='store_true')
    parser.add_argument('--colors', help="colored output", action='store_true')
    parser.add_argument('--show-stats', help="show stats", action='store_true')
    args = parser.parse_args()

    with open(args.file, 'rb') as f:
        out, stats = pretty_print(f.read(), show_merged=args.show_merged, show_lookahead_barriers=args.show_lookahead_barriers, colors=args.colors)
        print(out)
        if args.show_stats: print(stats, file=sys.stderr)
