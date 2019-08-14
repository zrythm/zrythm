/*
    This file is part of m.css.

    Copyright © 2017, 2018, 2019 Vladimír Vondruš <mosra@centrum.cz>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

"use strict"; /* it summons the Cthulhu in a proper way, they say */

const { Search } = require('../search.js');

const assert = require('assert');
const fs = require('fs');
const path = require('path');
const { StringDecoder } = require('string_decoder');

/* HTML escaping with RTL workarounds */
{
    assert.equal(Search.escape("foo()"), "foo()");
    assert.equal(Search.escapeForRtl("foo()"), "foo(&lrm;)&lrm;");

    assert.equal(Search.escape("Dir/"), "Dir/");
    /* Not sure why / and & has to be escaped from both sides */
    assert.equal(Search.escapeForRtl("Dir/"), "Dir&lrm;/&lrm;");

    assert.equal(Search.escape("foo() &&"), "foo() &amp;&amp;");
    assert.equal(Search.escapeForRtl("foo() &&"), "foo(&lrm;)&lrm; &lrm;&amp;&lrm;&lrm;&amp;&lrm;");

    assert.equal(Search.escape("operator=()"), "operator=()");
    assert.equal(Search.escapeForRtl("operator=()"), "operator&lrm;=(&lrm;)&lrm;");

    assert.equal(Search.escape("NS::Class<int>"), "NS::Class&lt;int&gt;");
    assert.equal(Search.escapeForRtl("NS::Class<int>"), "NS&lrm;:&lrm;:Class&lt;int&lrm;&gt;&lrm;");
}

/* UTF-8 en/decoding */
{
    assert.equal(Search.fromUtf8(Search.toUtf8("hýždě bříza")), "hýždě bříza");
}

/* Simple base85 decoding */
{
    let b85 = 'Xk~0{Zy-ZbL0VZLcW-iRWFa9T';
    let buf = Search.base85decode(b85);
    assert.equal(Buffer.from(buf).toString(), "hello CRAZY world!!!");
}

/* Unpadded base85 decoding */
{
    let b85 = 'Xk~0{Zy-ZbL0VZLcW-iRWFa9Te';
    let buf = Search.base85decode(b85);
    assert.ok(typeof buf === 'undefined');
}

/* Mess in base85 is interpreted as zeros */
{
    let b85 = '\'".: ';
    let buf = Search.base85decode(b85);
    assert.deepEqual(Buffer.from(buf), Buffer.from([0, 0, 0, 0]));
}

/* Verify that base85-decoded file is equivalent to the binary */
{
    let binary = fs.readFileSync(path.join(__dirname, "js-test-data/searchdata.bin"));
    assert.equal(binary.byteLength, 638);
    let b85 = fs.readFileSync(path.join(__dirname, "js-test-data/searchdata.b85"), {encoding: 'utf-8'});
    assert.deepEqual(new DataView(binary.buffer.slice(binary.byteOffset, binary.byteOffset + binary.byteLength)), new DataView(Search.base85decode(b85), 0, binary.byteLength));
}

/* Opening a too short file */
{
    let buffer = fs.readFileSync(path.join(__dirname, "js-test-data/short.bin"));
    assert.ok(!Search.init(buffer.buffer.slice(buffer.byteOffset, buffer.byteOffset + buffer.byteLength)));
}

/* Opening file with wrong signature */
{
    let buffer = fs.readFileSync(path.join(__dirname, "js-test-data/wrong-magic.bin"));
    assert.ok(!Search.init(buffer.buffer.slice(buffer.byteOffset, buffer.byteOffset + buffer.byteLength)));
}

/* Opening file with  */
{
    let buffer = fs.readFileSync(path.join(__dirname, "js-test-data/wrong-version.bin"));
    assert.ok(!Search.init(buffer.buffer.slice(buffer.byteOffset, buffer.byteOffset + buffer.byteLength)));
}

/* Search with empty data */
{
    let buffer = fs.readFileSync(path.join(__dirname, "js-test-data/empty.bin"));
    assert.ok(Search.init(buffer.buffer.slice(buffer.byteOffset, buffer.byteOffset + buffer.byteLength)));
    assert.equal(Search.symbolCount, 0);
    assert.deepEqual(Search.search(''), [[], '']);
}

/* Search */
{
    let buffer = fs.readFileSync(path.join(__dirname, "js-test-data/searchdata.bin"));
    assert.ok(Search.init(buffer.buffer.slice(buffer.byteOffset, buffer.byteOffset + buffer.byteLength)));
    assert.equal(Search.dataSize, 638);
    assert.equal(Search.symbolCount, 7);
    assert.equal(Search.maxResults, 100);

    /* Blow up */
    let resultsForM = [[
        { name: 'Math',
          url: 'namespaceMath.html',
          flags: 16,
          suffixLength: 3 },
        { name: 'Math::min(int, int)',
          url: 'namespaceMath.html#min',
          flags: 105,
          suffixLength: 12 },
        { name: 'Math::Vector::min() const',
          url: 'classMath_1_1Vector.html#min',
          flags: 105,
          suffixLength: 10 },
        { name: 'Math::Range::min() const',
          url: 'classMath_1_1Range.html#min',
          flags: 109,
          suffixLength: 10 }], ''];
    assert.deepEqual(Search.search('m'), resultsForM);

    /* Add more characters */
    assert.deepEqual(Search.search('min'), [[
        { name: 'Math::min(int, int)',
          url: 'namespaceMath.html#min',
          flags: 105,
          suffixLength: 10 },
        { name: 'Math::Vector::min() const',
          url: 'classMath_1_1Vector.html#min',
          flags: 105,
          suffixLength: 8 },
        { name: 'Math::Range::min() const',
          url: 'classMath_1_1Range.html#min',
          flags: 109,
          suffixLength: 8 }], '()']);

    /* Go back, get the same thing */
    assert.deepEqual(Search.search('m'), resultsForM);

    /* Search for something else */
    let resultsForVec = [[
        { name: 'Math::Vector',
          url: 'classMath_1_1Vector.html',
          flags: 40|2, /* Deprecated */
          suffixLength: 3 }], 'tor'];
    assert.deepEqual(Search.search('vec'), resultsForVec);

    /* Uppercase things and spaces */
    assert.deepEqual(Search.search(' Vec  '), resultsForVec);

    /* Not found */
    assert.deepEqual(Search.search('pizza'), [[], '']);

    /* UTF-8 decoding */
    assert.deepEqual(Search.search('su'), [[
        { name: Search.toUtf8('Page » Subpage'),
          url: 'subpage.html',
          flags: 192,
          suffixLength: 5 }], 'bpage']);

    /* Alias */
    assert.deepEqual(Search.search('r'), [[
        { name: 'Rectangle::Rect()',
          alias: 'Math::Range',
          url: 'classMath_1_1Range.html',
          flags: 40,
          suffixLength: 5 },
        { name: 'Math::Range',
          url: 'classMath_1_1Range.html',
          flags: 40,
          suffixLength: 4 },
        { name: 'Rectangle',
          alias: 'Math::Range',
          url: 'classMath_1_1Range.html',
          flags: 40,
          suffixLength: 8 }], '']);
}

/* Search, limiting the results to 3 */
{
    let buffer = fs.readFileSync(path.join(__dirname, "js-test-data/searchdata.bin"));
    assert.ok(Search.init(buffer.buffer.slice(buffer.byteOffset, buffer.byteOffset + buffer.byteLength), 3));
    assert.equal(Search.dataSize, 638);
    assert.equal(Search.symbolCount, 7);
    assert.equal(Search.maxResults, 3);
    assert.deepEqual(Search.search('m'), [[
        { name: 'Math',
          url: 'namespaceMath.html',
          flags: 16,
          suffixLength: 3 },
        { name: 'Math::min(int, int)',
          url: 'namespaceMath.html#min',
          flags: 105,
          suffixLength: 12 },
        { name: 'Math::Vector::min() const',
          url: 'classMath_1_1Vector.html#min',
          flags: 105,
          suffixLength: 10 }], '']);
}

/* Search loaded from a base85-encoded file should work properly */
{
    let b85 = fs.readFileSync(path.join(__dirname, "js-test-data/searchdata.b85"), {encoding: 'utf-8'});
    assert.ok(Search.load(b85));
    assert.equal(Search.dataSize, 640); /* some padding on the end, that's okay */
    assert.equal(Search.symbolCount, 7);
    assert.equal(Search.maxResults, 100);
    assert.deepEqual(Search.search('min'), [[
        { name: 'Math::min(int, int)',
          url: 'namespaceMath.html#min',
          flags: 105,
          suffixLength: 10 },
        { name: 'Math::Vector::min() const',
          url: 'classMath_1_1Vector.html#min',
          flags: 105,
          suffixLength: 8 },
        { name: 'Math::Range::min() const',
          url: 'classMath_1_1Range.html#min',
          flags: 109,
          suffixLength: 8 }], '()']);
}

/* Search, Unicode */
{
    let buffer = fs.readFileSync(path.join(__dirname, "js-test-data/unicode.bin"));
    assert.ok(Search.init(buffer.buffer.slice(buffer.byteOffset, buffer.byteOffset + buffer.byteLength)));
    assert.equal(Search.dataSize, 124);
    assert.equal(Search.symbolCount, 2);
    /* Both "Hýždě" and "Hárá" have common autocompletion to "h\xA1", which is
       not valid UTF-8, so it has to get truncated */
    assert.deepEqual(Search.search('h'), [[
        { name: Search.toUtf8('Hárá'),
          url: '#b',
          flags: 192,
          suffixLength: 5 },
        { name: Search.toUtf8('Hýždě'),
          url: '#a',
          flags: 192,
          suffixLength: 7 }], '']);
    /* These autocompletions are valid UTF-8, so nothing gets truncated */
    assert.deepEqual(Search.search('hý'), [[
        { name: Search.toUtf8('Hýždě'),
          url: '#a',
          flags: 192,
          suffixLength: 5 }], 'ždě']);
    assert.deepEqual(Search.search('há'), [[
        { name: Search.toUtf8('Hárá'),
          url: '#b',
          flags: 192,
          suffixLength: 3 }], 'rá']);
}

/* Properly combine heavily nested URLs */
{
    let buffer = fs.readFileSync(path.join(__dirname, "js-test-data/nested.bin"));
    assert.ok(Search.init(buffer.buffer.slice(buffer.byteOffset, buffer.byteOffset + buffer.byteLength)));
    assert.equal(Search.dataSize, 295);
    assert.equal(Search.symbolCount, 4);
    assert.deepEqual(Search.search('geo'), [[
        { name: 'Magnum::Math::Geometry',
          url: 'namespaceMagnum_1_1Math_1_1Geometry.html',
          flags: 24,
          suffixLength: 5 }], 'metry']);

    assert.deepEqual(Search.search('ra'), [[
        { name: 'Magnum::Math::Range',
          url: 'classMagnum_1_1Math_1_1Range.html',
          flags: 40,
          suffixLength: 3 }], 'nge']);
}

/* Not testing Search.download() because the xmlhttprequest npm package is *crap* */
