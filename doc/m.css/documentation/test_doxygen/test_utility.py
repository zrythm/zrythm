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

import unittest
import html

from doxygen import add_wbr, fix_type_spacing

class Utility(unittest.TestCase):
    def test_add_wbr(self):
        self.assertEqual(add_wbr('Corrade::Containers'), 'Corrade::<wbr />Containers')
        self.assertEqual(add_wbr('CORRADE_TEST_MAIN()'), 'CORRADE_<wbr />TEST_<wbr />MAIN()')
        self.assertEqual(add_wbr('https://magnum.graphics/showcase/'), 'https:/<wbr />/<wbr />magnum.graphics/<wbr />showcase/<wbr />')
        self.assertEqual(add_wbr('<strong>a</strong>'), '<strong>a</strong>')

    def test_fix_type_spacing(self):
        def fix_escaped(str):
            return html.unescape(fix_type_spacing(html.escape(str)))

        self.assertEqual(fix_escaped('Foo< T, U > *'), 'Foo<T, U>*')
        self.assertEqual(fix_escaped('Foo< T, U > &'), 'Foo<T, U>&')
        self.assertEqual(fix_escaped('Foo< T&&U >'), 'Foo<T && U>')
