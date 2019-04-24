/* =========================================================================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

#ifndef BINARYDATA_H_79472171_INCLUDED
#define BINARYDATA_H_79472171_INCLUDED

namespace BinaryData
{
    extern const char*   admvicon_png;
    const int            admvicon_pngSize = 7629;

    extern const char*   check_check_png;
    const int            check_check_pngSize = 2937;

    extern const char*   check_false_png;
    const int            check_false_pngSize = 2822;

    extern const char*   check_true_png;
    const int            check_true_pngSize = 2963;

    // Points to the start of a list of resource names.
    extern const char* namedResourceList[];

    // Number of elements in the namedResourceList array.
    const int namedResourceListSize = 4;

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding data and its size (or a null pointer if the name isn't found).
    const char* getNamedResource (const char* resourceNameUTF8, int& dataSizeInBytes) throw();
}

#endif
