
///=============================================================================
///    itoa.h - A C++ header to implement itoa()
///    Created: 11 Feb 2015 5:08:51pm
///    Author:  Jeff-Russ
///=============================================================================
#pragma once

#include <iostream>
using namespace std;

// A utility function to reverse a string:
void reverse(char str[], int length)
{
    int start = 0;
    int end = length -1;
    while (start < end)
    {   swap(*(str+start), *(str+end));
        start++;
        end--;
    }
};

char* itoa(int num, char* str, int base)
{
    int i = 0;
    bool isNegative = false;    
    
    if (num == 0)               // Handle 0 explicitly, otherwise
    {   str[i++] = '0';         // empty string is printed for 0
        str[i] = '\0';
        return str;
    }     
                                // In standard itoa(), negative
    if (num < 0 && base == 10)  // numbers are handled only with
    {   isNegative = true;      // base 10. Otherwise numbers
        num = -num;             // are considered unsigned.
    }
    while (num != 0)            
    {   int rem = num % base;   // Process individual digits
        str[i++] = (rem > 9)? (rem-10) + 'a' : rem + '0';
        num = num/base;
    }
    if (isNegative)             // If number is negative, append '-'
        str[i++] = '-';
    
    str[i] = '\0';              // Append string terminator
    reverse(str, i);            // Reverse the string
    return str;
};
