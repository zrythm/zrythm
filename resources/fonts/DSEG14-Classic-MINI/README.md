DSEG Font Family (v0.44)
====

## Overview

DSEG is a free font which imitate LCD Display (7SEG, 14SEG, Weather icons etc.).
DSEG have special features:

 - Includes the roman-alphabet and symbol glyphs.
 - Many types(over 50) are available.
 - Licensed under [SIL OPEN FONT LICENSE Version 1.1](http://scripts.sil.org/OFL)

![DSEG Image](http://www.keshikan.net/img/DSEG_sample_github.png)

## Sample
![DSEG Sample Image](http://www.keshikan.net/img/DSEG_weather_sample.png)

## Usage

 - Colon and Space

   ![DSEG usage 1](http://www.keshikan.net/img/dseg_usage1.png)

 - Period

   ![DSEG usage 2](http://www.keshikan.net/img/dseg_usage2.png)

 - All-off (Exclamation)

   ![DSEG usage 3](http://www.keshikan.net/img/dseg_usage3.png)

 - All-on ("8" or Tilda)

   ![DSEG usage 4](http://www.keshikan.net/img/dseg_usage4.png)

For more information, visit DSEG support page.

 - English:[http://www.keshikan.net/fonts-e.html](http://www.keshikan.net/fonts-e.html)
 - Japanese:[http://www.keshikan.net/fonts.html](http://www.keshikan.net/fonts.html)

## Install for Node.js

    $ npm install dseg

## Build *ttf from *.sfd

    $ make

## Changelog

 - v0.45(2018/01/09)
    - Added makefile and build script. (Merged #8 #9 . Thanks to [alexmyczko](https://github.com/alexmyczko))

 - v0.44(2018/01/02)
    - Modified colon character position for balancing in Italic style. See below.  
![DSEG v044](http://www.keshikan.net/img/dseg_mod_v044.png)
    - Added License metadata to *.ttf .
    - Changed file name of *.sfd to match it's font-name.
  
 - v0.43(2017/08/15)
    - Added package.json and registerd npmjs.com. ([merged pull request #5](https://github.com/keshikan/DSEG/pull/5))

 - v0.42(2017/04/27)
    - Added WOFF2 file format.
    - Added [extended metadata block](https://www.w3.org/TR/WOFF/#Metadata)  to *.woff and *.woff2. 
  
 - v0.41(2017/01/07)
    - Assigned all-segment-off status to exclamation mark(U+0021).

 - v0.40(2017/01/06)
    - First released in Github.
    - Added weather icons "DSEGWeather".
    - The license has been changed to the [SIL OPEN FONT LICENSE Version 1.1](http://scripts.sil.org/OFL).

## License

- Any font files(*.ttf, *.woff, *.sfd) are licensed under the [SIL OPEN FONT LICENSE Version 1.1](http://scripts.sil.org/OFL)

