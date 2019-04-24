/* =========================================================================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

#ifndef BINARYDATA_H_21227425_INCLUDED
#define BINARYDATA_H_21227425_INCLUDED

namespace BinaryData
{
    extern const char*   hifilofi_jpg;
    const int            hifilofi_jpgSize = 7971;

    extern const char*   brushed_aluminium_png;
    const int            brushed_aluminium_pngSize = 14724;

    // Points to the start of a list of resource names.
    extern const char* namedResourceList[];

    // Number of elements in the namedResourceList array.
    const int namedResourceListSize = 2;

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding data and its size (or a null pointer if the name isn't found).
    const char* getNamedResource (const char* resourceNameUTF8, int& dataSizeInBytes) throw();
}

#endif
