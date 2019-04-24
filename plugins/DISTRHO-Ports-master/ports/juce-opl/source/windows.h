
///=============================================================================
///    windows.h - as needed re-definition of windows.h functions for non-win
///    Created: 11 Feb 2015 5:08:51pm
///    Author:  Jeff-Russ
///=============================================================================

#pragma once

#include <stdint.h>


// PVOID  - A pointer to any type.
//          This type is declared in WinNT.h as follows:
typedef void* PVOID;

// DWORDLONG A 64-bit unsigned integer.
//           This type is declared in IntSafe.h as follows:
typedef uint64_t DWORDLONG;


#define WINAPI

// DWORD  - A 32-bit unsigned integer.
//          This type is declared in IntSafe.h as follows:
typedef uint32_t DWORD;

// WCHAR  - A 16-bit unicode character
//           This type is declared in WinNT.h as follows:
typedef int16_t WCHAR;

// HANDLE - A handle to an object.
//          This type is declared in WinDef.h as follows:
typedef /*PVOID*/ DWORD HANDLE;

DWORD STD_INPUT_HANDLE = -10;
DWORD STD_OUTPUT_HANDLE = -11;
DWORD STD_ERROR_HANDLE = -12;


#define MAX_PATH    PATH_MAX

// BYTE  -   A byte (8-bits)
//           This type is declared in WinDef.h as follows:
typedef unsigned char BYTE;

// WORD  - A 16-bit unsigned integer.
//          This type is declared in WinDef.h as follows:
typedef uint16_t WORD;

// BOOL  -  A boolean variable
//          This type is declared in WinDef.h as follows:
typedef BYTE BOOL;

DWORD GetStdHandle (DWORD handle) { return handle; }

void WriteConsole(DWORD conout,const char* strPtr,
                  DWORD count, DWORD* countAdr, int* nul)
{
    std::string msgType = "NOTICE: ";
    
    if (conout == -10)
        msgType = "INPUT: ";
    else if (conout == -11)
        msgType = "OUTPUT: ";
    else if (conout == -12)
        msgType = "ERROR: ";
    else
        msgType = "NOTICE: ";
    
    std::string message (const char* str, uint32_t count);
    
    std::cout << std::endl << msgType << message;
}

void AllocConsole()
{
    std::cout << std::endl << "=== Starting debug console....." << std::endl;
}
void FreeConsole()
{
    std::cout << std::endl << "=== Exited debug console ===" << std::endl;
}


