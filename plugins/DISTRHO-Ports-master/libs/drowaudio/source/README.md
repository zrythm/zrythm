# dRowAudio - A JUCE module for high level audio application development.

dRowAudio is a 3rd party JUCE module designed for rapid audio application development. It contains classes for audio processing and gui elements. Additionally there are several wrappers around 3rd party libraries including cURL, FFTReal and SoundTouch. dRowAudio is written in the strict JUCE style, closely following the style guide set out at [JUCE Coding Standards][1].

dRowAudio is hosted on Github at [https://github.com/drowaudio/drowaudio][2]

The online documentation is at [http://drowaudio.co.uk/docs/][3]

## Platforms

All platforms supported by JUCE are also supported by dRowAudio. Currently these
platforms include:

- **Windows**: Applications and VST/RTAS/NPAPI/ActiveX plugins can be built 
using MS Visual Studio. The results are all fully compatible with Windows
XP, Vista or Windows 7.

- **Mac OS X**: Applications and VST/AudioUnit/RTAS/NPAPI plugins with Xcode.

- **GNU/Linux**: Applications and plugins can be built for any kernel 2.6 or
later.

- **iOS**: Native iPhone and iPad apps.

- **Android**: Supported.

## Prerequisites

This documentation assumes that the reader has a working knowledge of JUCE.

## External Modules

In order to use the cURL classes you will need to link to the cURL library. This is included as part of Mac OSX, for Windows there pre-built 32-bit binaries or you can download the library yourself for the most recent version.

Although some aspects of dRowAudio rely on other 3rd party modules such as [SoundTouch][5] and [FFTReal][6], these are included as part of the module so no external linking is required. Their use should be transparent to the user.

## Integration

dRowAudio requires recent versions of JUCE. It won't work with versions 2.36 or
earlier. To use the library it is necessary to first download JUCE to a
location where your development environment can find it. Or, you can use your
existing installation of JUCE.

To use the module simply include it, or a symbolic link to it, in your juce/modules folder. Simply them run the Introjucer as normal and tick the dRowAudio module. Config flags are provided to disable some functionality if not required.

## License

Copyright (C) 2013 by David Rowland ([e-mail][0])

dRowAudio is provided under the terms of The MIT License (MIT):

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Some portions of the software including but not limited to [SoundTouch][5] and [FFTReal][6] are included with in the repository but released under separate licences. Please see the individual source files for details.

[0]: mailto:dave@drowaudio.co.uk "David Rowland (Email)"
[1]: http://www.rawmaterialsoftware.com/wiki/index.php/Coding_Standards
[2]: https://github.com/drowaudio/drowaudio
[3]: http://drowaudio.co.uk/docs/
[5]: http://www.surina.net/soundtouch/index.html
[6]: http://ldesoras.free.fr/prod.html
[7]: http://www.gnu.org/licenses/gpl-2.0.html
[8]: http://www.opensource.org/licenses/mit-license.html "The MIT License"