/* =========================================================================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

#ifndef BINARYDATA_H_12816941_INCLUDED
#define BINARYDATA_H_12816941_INCLUDED

namespace BinaryData
{
    extern const char*   knoblsd_png;
    const int            knoblsd_pngSize = 214673;

    extern const char*   knobssd_png;
    const int            knobssd_pngSize = 174727;

    extern const char*   legato_png;
    const int            legato_pngSize = 7913;

    extern const char*   voices_png;
    const int            voices_pngSize = 3496;

    extern const char*   button_png;
    const int            button_pngSize = 1794;

    extern const char*   main_png;
    const int            main_pngSize = 147135;

    // Points to the start of a list of resource names.
    extern const char* namedResourceList[];

    // Number of elements in the namedResourceList array.
    const int namedResourceListSize = 6;

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding data and its size (or a null pointer if the name isn't found).
    const char* getNamedResource (const char* resourceNameUTF8, int& dataSizeInBytes) throw();
}

#endif
